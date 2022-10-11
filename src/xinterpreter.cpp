//===----------- xinterpreter.cpp - The Xeus interpreter wrapper ----------===//
//
// Licensed under the Apache License v2.0.
// SPDX-License-Identifier: Apache-2.0
//
// The full license is in the file LICENSE, distributed with this software.
//
//===----------------------------------------------------------------------===//
//
// This file contains the interpreter wrapper implementation used by Xeus to
// communicate with Jupyter. It bridges clang-repl and Jupyter.
//
//===----------------------------------------------------------------------===//

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <memory>
#include <regex>
#include <sstream>
#include <vector>

#include "Python.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticPrinter.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/Support/TargetSelect.h"

#include "xeus-clang-repl/xbuffer.hpp"
#include "xeus-clang-repl/xeus_clang-repl_config.hpp"
#include "xeus-clang-repl/xinput.hpp"
#include "xeus-clang-repl/xinterpreter.hpp"
#include "xeus-clang-repl/xmagics.hpp"
#include "xeus-clang-repl/xparser.hpp"
#include "xeus-clang-repl/xsystem.hpp"

#include "xmagics/os.hpp"
#include "xmagics/pythonexec.hpp"

using namespace std::placeholders;
std::string DiagnosticOutput;
llvm::raw_string_ostream DiagnosticsOS(DiagnosticOutput);
auto DiagPrinter = std::make_unique<clang::TextDiagnosticPrinter>(
    DiagnosticsOS, new clang::DiagnosticOptions());

///\returns true on error.
static bool ProcessCode(clang::Interpreter &Interp, const std::string &code,
                        llvm::raw_string_ostream &error_stream) {
  if (xcpp::pythonexec::python_check_for_initialisation()) {
    std::string newPythonInts =
        xcpp::pythonexec::transfer_python_ints_utility();
    llvm::cantFail(Interp.ParseAndExecute(newPythonInts));
    std::string newPythonLists =
        xcpp::pythonexec::transfer_python_lists_utility();
    llvm::cantFail(Interp.ParseAndExecute(newPythonLists));
  }

  auto PTU = Interp.Parse(code);
  if (!PTU) {
    auto Err = PTU.takeError();
    error_stream << DiagnosticsOS.str();
    // avoid printing the "Parsing failed error"
    // llvm::logAllUnhandledErrors(std::move(Err), error_stream, "error: ");
    return true;
  }
  if (PTU->TheModule) {
    llvm::Error ex = Interp.Execute(*PTU);
    error_stream << DiagnosticsOS.str();
    if (code.substr(0, 3) == "int") {
      for (clang::Decl *D : PTU->TUPart->decls()) {
        if (clang::VarDecl *VD = llvm::dyn_cast<clang::VarDecl>(D)) {
          auto Name = VD->getNameAsString();
          auto Addr = Interp.getSymbolAddress(clang::GlobalDecl(VD));
          if (!Addr) {
            llvm::logAllUnhandledErrors(std::move(Addr.takeError()),
                                        error_stream, "error: ");
            return true;
          }
          void *AddrVP = (void *)*Addr;
          // printf("Value at '%p' is:'%d'\n", AddrVP, *(int*)AddrVP);
          xcpp::pythonexec::update_python_dict_var(Name.c_str(),
                                                   *(int *)AddrVP);
        }
      }
    }

    else if (code.substr(0, 16) == "std::vector<int>") {
      for (clang::Decl *D : PTU->TUPart->decls()) {
        if (clang::VarDecl *VD = llvm::dyn_cast<clang::VarDecl>(D)) {
          auto Name = VD->getNameAsString();
          auto Addr = Interp.getSymbolAddress(clang::GlobalDecl(VD));
          if (!Addr) {
            llvm::logAllUnhandledErrors(std::move(Addr.takeError()),
                                        error_stream, "error: ");
            return true;
          }
          void *AddrVP = (void *)*Addr;
          xcpp::pythonexec::update_python_dict_var_vector(
              Name.c_str(), *(std::vector<int> *)AddrVP);
        }
      }
    }

    llvm::logAllUnhandledErrors(std::move(ex), error_stream, "error: ");
    return false;
  }
  return false;
}

using Args = std::vector<const char *>;
static std::unique_ptr<clang::Interpreter>
createInterpreter(const Args &ExtraArgs = {},
                  clang::DiagnosticConsumer *Client = nullptr) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  Args ClangArgs = {"-Xclang", "-emit-llvm-only",
                    "-Xclang", "-diagnostic-log-file",
                    "-Xclang", "-",
                    "-xc++"};
  ClangArgs.insert(ClangArgs.end(), ExtraArgs.begin(), ExtraArgs.end());
  auto CI = cantFail(clang::IncrementalCompilerBuilder::create(ClangArgs));
  if (Client)
    CI->getDiagnostics().setClient(Client, /*ShouldOwnClient=*/false);
  return cantFail(clang::Interpreter::create(std::move(CI)));
}

namespace xcpp {
void interpreter::configure_impl() {}

interpreter::interpreter(int argc, const char *const *argv)
    : m_interpreter(std::move(createInterpreter(Args(argv + 2, argv + argc - 3),
                                                DiagPrinter.get()))),
      //          m_input_validator(),
      m_version(get_stdopt(argc, argv)), // Extract C++ language standard
                                         // version from command-line option
      xmagics(), p_cout_strbuf(nullptr), p_cerr_strbuf(nullptr),
      m_cout_buffer(std::bind(&interpreter::publish_stdout, this, _1)),
      m_cerr_buffer(std::bind(&interpreter::publish_stderr, this, _1)) {
  // Bootstrap the execution engine
  redirect_output();
  init_preamble();
  init_magic();
}

interpreter::~interpreter() { restore_output(); }

nl::json interpreter::execute_request_impl(int /*execution_counter*/,
                                           const std::string &code, bool silent,
                                           bool /*store_history*/,
                                           nl::json /*user_expressions*/,
                                           bool allow_stdin) {
  nl::json kernel_res;

  // Check for magics
  for (auto &pre : preamble_manager.preamble) {
    if (pre.second.is_match(code)) {
      pre.second.apply(code, kernel_res);
      return kernel_res;
    }
  }

  // Split code from includes
  auto blocks = split_from_includes(code.c_str());

  auto errorlevel = 0;

  std::string ename;
  std::string evalue;
  bool compilation_result;

  // If silent is set to true, temporarily dismiss all std::cerr and
  // std::cout outputs resulting from `m_interpreter.process`.

  auto cout_strbuf = std::cout.rdbuf();
  auto cerr_strbuf = std::cerr.rdbuf();

  if (silent) {
    auto null = xnull();
    std::cout.rdbuf(&null);
    std::cerr.rdbuf(&null);
  }

  // Scope guard performing the temporary redirection of input requests.
  auto input_guard = input_redirection(allow_stdin);

  for (const auto &block : blocks) {
    // Attempt normal evaluation
    std::string error_message;
    llvm::raw_string_ostream error_stream(error_message);
    try {
      compilation_result = ProcessCode(*m_interpreter, block, error_stream);
      redirect_output();
    }

    catch (std::exception &e) {
      errorlevel = 1;
      ename = "Standard Exception";
      evalue = e.what();
    } catch (...) {
      errorlevel = 1;
      ename = "Error";
    }

    // if (compilation_result != cling::Interpreter::kSuccess)
    if (compilation_result) {
      errorlevel = 1;
      // send the errors directly to std::cerr
      ename = "";
      std::cerr << error_stream.str();
    }

    // If an error was encountered, don't attempt further execution
    if (errorlevel) {
      error_stream.str().clear();
      DiagnosticsOS.str().clear();
      break;
    }
  }

  // Flush streams
  std::cout << std::flush;
  std::cerr << std::flush;

  // Reset non-silent output buffers
  if (silent) {
    std::cout.rdbuf(cout_strbuf);
    std::cerr.rdbuf(cerr_strbuf);
  }

  // Depending of error level, publish execution result or execution
  // error, and compose execute_reply message.
  if (errorlevel) {
    // Classic Notebook does not make use of the "evalue" or "ename"
    // fields, and only displays the traceback.
    //
    // JupyterLab displays the "{ename}: {evalue}" if the traceback is
    // empty.
    std::vector<std::string> traceback({ename + " " + evalue});
    if (!silent) {
      publish_execution_error(ename, evalue, traceback);
    }

    // Compose execute_reply message.
    kernel_res["status"] = "error";
    kernel_res["ename"] = ename;
    kernel_res["evalue"] = evalue;
    kernel_res["traceback"] = traceback;
  } else {
    // Publish a mime bundle for the last return value if
    // the semicolon was omitted.
    //   if (!silent && /*output.hasValue() &&*/ trim(blocks.back()).back() !=
    //   ';')
    //   {
    //     nl::json pub_data = nl::json::object();
    //     pub_data["text/plain"] = "";
    //     publish_execution_result(execution_counter, std::move(pub_data),
    //     nl::json::object());
    //   }

    // Compose execute_reply message.
    kernel_res["status"] = "ok";
    kernel_res["payload"] = nl::json::array();
    kernel_res["user_expressions"] = nl::json::object();
  }
  return kernel_res;
}

nl::json interpreter::complete_request_impl(const std::string & /*code*/,
                                            int /*cursor_pos*/) {
  return {};
  // std::vector<std::string> result;
  // cling::Interpreter::CompilationResult compilation_result;
  // nl::json kernel_res;

  // // split the input to have only the word in the back of the cursor
  // std::string delims = " \t\n`!@#$^&*()=+[{]}\\|;:\'\",<>?.";
  // std::size_t _cursor_pos = cursor_pos;
  // auto text = split_line(code, delims, _cursor_pos);
  // std::string to_complete = text.back().c_str();

  // compilation_result = m_interpreter.codeComplete(code.c_str(), _cursor_pos,
  // result);

  // // change the print result
  // for (auto& r : result)
  // {
  //     // remove the definition at the beginning (for example [#int#])
  //     r = std::regex_replace(r, std::regex("\\[\\#.*\\#\\]"), "");
  //     // remove the variable name in <#type name#>
  //     r = std::regex_replace(r, std::regex("(\\ |\\*)+(\\w+)(\\#\\>)"),
  //     "$1$3");
  //     // remove unnecessary space at the end of <#type   #>
  //     r = std::regex_replace(r, std::regex("\\ *(\\#\\>)"), "$1");
  //     // remove <# #> to keep only the type
  //     r = std::regex_replace(r, std::regex("\\<\\#([^#>]*)\\#\\>"), "$1");
  // }

  // kernel_res["matches"] = result;
  // kernel_res["cursor_start"] = cursor_pos - to_complete.length();
  // kernel_res["cursor_end"] = cursor_pos;
  // kernel_res["metadata"] = nl::json::object();
  // kernel_res["status"] = "ok";
  // return kernel_res;
}

nl::json interpreter::inspect_request_impl(const std::string &code,
                                           int cursor_pos,
                                           int /*detail_level*/) {
  nl::json kernel_res;

  auto dummy = code.substr(0, cursor_pos);
  // TODO: same pattern as in inspect function (keep only one)
  std::string exp = R"(\w*(?:\:{2}|\<.*\>|\(.*\)|\[.*\])?)";
  std::regex re_method{"(" + exp + R"(\.?)*$)"};
  std::smatch magic;
  if (std::regex_search(dummy, magic, re_method)) {
    // inspect(magic[0], kernel_res, m_interpreter);
  }
  return kernel_res;
}

nl::json interpreter::is_complete_request_impl(const std::string & /*code*/) {
  nl::json kernel_res;
  kernel_res["status"] = "complete";
  return kernel_res;

  // m_input_validator.reset();
  // cling::InputValidator::ValidationResult Res =
  // m_input_validator.validate(code); if (Res ==
  // cling::InputValidator::kComplete)
  // {
  //      kernel_res["status"] = "complete";
  // }
  // else if (Res == cling::InputValidator::kIncomplete)
  // {
  //     kernel_res["status"] = "incomplete";
  // }
  // else if (Res == cling::InputValidator::kMismatch)
  // {
  //     kernel_res["status"] = "invalid";
  // }
  // else
  // {
  //     kernel_res["status"] = "unknown";
  // }
  // kernel_res["indent"] = "";
  // return kernel_res;
}

nl::json interpreter::kernel_info_request_impl() {
  nl::json result;
  result["implementation"] = "xeus-clang-repl";
  result["implementation_version"] = XEUS_CLANG_REPL_VERSION;

  /* The jupyter-console banner for xeus-clang-repl is the following:
    __  _____ _   _ ___
    \ \/ / _ \ | | / __|
     >  <  __/ |_| \__ \
    /_/\_\___|\__,_|___/

    xeus-clang-repl: a Jupyter Kernel C++ - based on clang-repl
  */

  std::string banner =
      ""
      "  __  _____ _   _ ___\n"
      "  \\ \\/ / _ \\ | | / __|\n"
      "   >  <  __/ |_| \\__ \\\n"
      "  /_/\\_\\___|\\__,_|___/\n"
      "\n"
      "  xeus-clang-repl: a Jupyter Kernel C++ - based on clang-repl\n"
      "  C++";
  banner.append(m_version);
  result["banner"] = banner;
  result["language_info"]["name"] = "c++";
  result["language_info"]["version"] = m_version;
  result["language_info"]["mimetype"] = "text/x-c++src";
  result["language_info"]["codemirror_mode"] = "text/x-c++src";
  result["language_info"]["file_extension"] = ".cpp";
  result["help_links"] = nl::json::array();
  result["help_links"][0] =
      nl::json::object({{"text", "clang-repl Reference"},
                        {"url", "https://github.com/llvm/llvm-project"}});
  result["status"] = "ok";
  return result;
}

void interpreter::shutdown_request_impl() { restore_output(); }

static std::string c_format(const char *format, std::va_list args) {
  // Call vsnprintf once to determine the required buffer length. The
  // return value is the number of characters _excluding_ the null byte.
  std::va_list args_bufsz;
  va_copy(args_bufsz, args);
  std::size_t bufsz = vsnprintf(NULL, 0, format, args_bufsz);
  va_end(args_bufsz);

  // Create an empty string of that size.
  std::string s(bufsz, 0);

  // Now format the data into this string and return it.
  std::va_list args_format;
  va_copy(args_format, args);
  // The second parameter is the maximum number of bytes that vsnprintf
  // will write _including_ the  terminating null byte.
  vsnprintf(&s[0], s.size() + 1, format, args_format);
  va_end(args_format);

  return s;
}

static int printf_jit(const char *format, ...) {
  std::va_list args;
  va_start(args, format);

  std::string buf = c_format(format, args);
  std::cout << buf;

  va_end(args);

  return buf.size();
}

static int fprintf_jit(std::FILE *stream, const char *format, ...) {
  std::va_list args;
  va_start(args, format);

  int ret;
  if (stream == stdout || stream == stderr) {
    std::string buf = c_format(format, args);
    if (stream == stdout) {
      std::cout << buf;
    } else if (stream == stderr) {
      std::cerr << buf;
    }
    ret = buf.size();
  } else {
    // Just forward to vfprintf.
    ret = vfprintf(stream, format, args);
  }

  va_end(args);

  return ret;
}

static void injectSymbol(llvm::StringRef LinkerMangledName,
                         llvm::JITTargetAddress KnownAddr,
                         clang::Interpreter &Interp) {
  using namespace llvm;
  using namespace llvm::orc;

  auto Symbol =
      Interp.getSymbolAddress(LinkerMangledName); //, /*IncludeFromHost=*/true);
  if (Error Err = Symbol.takeError()) {
    logAllUnhandledErrors(std::move(Err), errs(),
                          "[IncrementalJIT] define() failed1: ");
    return;
  }

  // Nothing to define, we are redefining the same function. FIXME: Diagnose.
  if (*Symbol && (JITTargetAddress)*Symbol == KnownAddr)
    return;

  // Let's inject it
  bool Inserted;
  SymbolMap::iterator It;
  static llvm::orc::SymbolMap m_InjectedSymbols;

  llvm::orc::LLJIT *Jit =
      const_cast<llvm::orc::LLJIT *>(Interp.getExecutionEngine());
  JITDylib &DyLib = Jit->getMainJITDylib();

  std::tie(It, Inserted) = m_InjectedSymbols.try_emplace(
      Jit->getExecutionSession().intern(LinkerMangledName),
      JITEvaluatedSymbol(KnownAddr, JITSymbolFlags::Exported));
  assert(Inserted && "Why wasn't this found in the initial Jit lookup?");

  // We want to replace a symbol with a custom provided one.
  if (Symbol && KnownAddr) {
    // The symbol be in the DyLib or in-process.
    if (auto Err = DyLib.remove({It->first})) {
      logAllUnhandledErrors(std::move(Err), errs(),
                            "[IncrementalJIT] define() failed2: ");
      return;
    }
  }

  if (Error Err = DyLib.define(absoluteSymbols({*It})))
    logAllUnhandledErrors(std::move(Err), errs(),
                          "[IncrementalJIT] define() failed3: ");
}

void interpreter::redirect_output() {
  p_cout_strbuf = std::cout.rdbuf();
  p_cerr_strbuf = std::cerr.rdbuf();

  std::cout.rdbuf(&m_cout_buffer);
  std::cerr.rdbuf(&m_cerr_buffer);

  // Inject versions of printf and fprintf that output to std::cout
  // and std::cerr (see implementation above).
  injectSymbol("printf", llvm::pointerToJITTargetAddress(printf_jit),
               *m_interpreter);
  injectSymbol("fprintf", llvm::pointerToJITTargetAddress(fprintf_jit),
               *m_interpreter);
  // llvm::sys::DynamicLibrary::AddSymbol("printf", (void*) &printf_jit);
  // llvm::sys::DynamicLibrary::AddSymbol("fprintf", (void*) &fprintf_jit);
}

void interpreter::restore_output() {
  std::cout.rdbuf(p_cout_strbuf);
  std::cerr.rdbuf(p_cerr_strbuf);

  // No need to remove the injected versions of [f]printf: As they forward
  // to std::cout and std::cerr, these are handled implicitly.
}

void interpreter::publish_stdout(const std::string &s) {
  publish_stream("stdout", s);
}

void interpreter::publish_stderr(const std::string &s) {
  publish_stream("stderr", s);
}

void interpreter::init_preamble() {
  // preamble_manager.register_preamble("introspection", new
  // xintrospection(m_interpreter));
  preamble_manager.register_preamble("magics", new xmagics_manager());
  preamble_manager.register_preamble("shell", new xsystem());
}

void interpreter::init_magic() {
  preamble_manager["magics"].get_cast<xmagics_manager>().register_magic(
      "file", writefile());
  preamble_manager["magics"].get_cast<xmagics_manager>().register_magic(
      "python", pythonexec());
}

std::string interpreter::get_stdopt(int argc, const char *const *argv) {
  std::string res = "11";
  for (int i = 0; i < argc; ++i) {
    std::string tmp(argv[i]);
    auto pos = tmp.find("-std=c++");
    if (pos != std::string::npos) {
      res = tmp.substr(pos + 8);
      break;
    }
  }
  return res;
}
} // namespace xcpp
