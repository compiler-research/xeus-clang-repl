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

using Args = std::vector<const char *>;
void* createInterpreter(const Args &ExtraArgs = {}) {
  Args ClangArgs = {/*"-xc++"*/};
  ClangArgs.insert(ClangArgs.end(), ExtraArgs.begin(), ExtraArgs.end());
  // FIXME: We should process the kernel input options and conditionally pass
  // the gpu args here.
  return Cpp::CreateInterpreter(ClangArgs, {"-cuda"});
}

namespace xcpp {
void interpreter::configure_impl() {}

interpreter::interpreter(int argc, const char *const *argv)
    : //          m_input_validator(),
      m_version(get_stdopt(argc, argv)), // Extract C++ language standard
                                         // version from command-line option
      xmagics(), p_cout_strbuf(nullptr), p_cerr_strbuf(nullptr),
      m_cout_buffer(std::bind(&interpreter::publish_stdout, this, _1)),
      m_cerr_buffer(std::bind(&interpreter::publish_stderr, this, _1)) {
  createInterpreter(Args(argv, argv + argc));
  redirect_output();
  // Bootstrap the execution engine
  init_preamble();
  init_magic();
}

interpreter::~interpreter() { restore_output(); }


std::string namespace_name(int namespace_number) {
  const std::string NAMESPACE_PREFIX = "__xns";
  return NAMESPACE_PREFIX + std::to_string(namespace_number);
}

std::string namespace_full_name(int namespace_number) {
  std::string result;
  for (int i = 1; i < namespace_number; ++i) {
    result += namespace_name(i) + "::";
  }
  result += namespace_name(namespace_number);
  return result;
}

std::string encapsulate_code(const std::string &code, int namespace_number) {
  return "\nnamespace " + namespace_full_name(namespace_number) + " {" + code + "}";
}

nl::json interpreter::execute_request_impl(int execution_counter,
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
    std::string err;
    std::string out;
    try {
      Cpp::BeginStdStreamCapture(Cpp::kStdErr);
      Cpp::BeginStdStreamCapture(Cpp::kStdOut);
      if (block.is_include) {
        compilation_result = Cpp::Process(block.code.c_str());
      } else {
        std::string encapsulated = encapsulate_code(block.code, execution_counter);
        compilation_result = Cpp::Process(encapsulated.c_str());
      }
      out = Cpp::EndStdStreamCapture();
      err = Cpp::EndStdStreamCapture();
      std::cout << out;
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
      std::cerr << err;

      // If an error was encountered, don't attempt further execution
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

void interpreter::redirect_output() {
  p_cout_strbuf = std::cout.rdbuf();
  p_cerr_strbuf = std::cerr.rdbuf();

  std::cout.rdbuf(&m_cout_buffer);
  std::cerr.rdbuf(&m_cerr_buffer);
}

void interpreter::restore_output() {
  std::cout.rdbuf(p_cout_strbuf);
  std::cerr.rdbuf(p_cerr_strbuf);
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
