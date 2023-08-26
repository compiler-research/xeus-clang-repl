//===------------- main.cpp - The xeus-clang-repl entry point -------------===//
//
// Licensed under the Apache License v2.0.
// SPDX-License-Identifier: Apache-2.0
//
// The full license is in the file LICENSE, distributed with this software.
//
//===----------------------------------------------------------------------===//
//
// This file is the entry point to the service connecting the clang-repl project
// and the Jupyter platform via the xeus protocol.
//
//===----------------------------------------------------------------------===//

#include "xeus/xkernel.hpp"
#include "xeus/xkernel_configuration.hpp"
#include "xeus/xserver_zmq.hpp"

#include "xeus-clang-repl/xeus_clang-repl_config.hpp"
#include "xeus-clang-repl/xinterpreter.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

bool should_print_version(int argc, char *argv[]) {
  for (int i = 0; i < argc; ++i) {
    if (std::string(argv[i]) == "--version") {
      return true;
    }
  }
  return false;
}

std::string extract_filename(int& argc, char *argv[]) {
  std::string res = "";
  for (int i = 0; i < argc; ++i) {
    if ((std::string(argv[i]) == "-f") && (i + 1 < argc)) {
      res = argv[i + 1];
      for (int j = i; j < argc - 2; ++j) {
        argv[j] = argv[j + 2];
      }
      argc -= 2;
      break;
    }
  }
  return res;
}

using interpreter_ptr = std::unique_ptr<xcpp::interpreter>;
interpreter_ptr build_interpreter(int argc, char **argv) {
  std::vector<const char*> interpreter_args;
  for (int i = 1; i < argc; i++) {
    if (argv[i] == "-f") {
      i++; // skip the value of -f which is a json file.
      continue;
    }
    interpreter_args.push_back(argv[i]);
  }
  interpreter_ptr interp_ptr = interpreter_ptr(
       new xcpp::interpreter(interpreter_args.size(), interpreter_args.data()));
  return interp_ptr;
}

int main(int argc, char *argv[]) {
  if (should_print_version(argc, argv)) {
    std::clog << "xcpp " << XEUS_CLANG_REPL_VERSION << std::endl;
    return 0;
  }

  // If we are called from the Jupyter launcher, silence all logging. This
  // is important for a JupyterHub configured with cleanup_servers = False:
  // Upon restart, spawned single-user servers keep running but without the
  // std* streams. When a user then tries to start a new kernel, xeus-clang-repl
  // will get a SIGPIPE when writing to any of these and exit.
  if (std::getenv("JPY_PARENT_PID") != NULL) {
    std::clog.setstate(std::ios_base::failbit);
  }

  std::string file_name = extract_filename(argc, argv);

  interpreter_ptr interpreter = build_interpreter(argc, argv);

  auto context = xeus::make_context<zmq::context_t>();

  if (!file_name.empty()) {
    xeus::xconfiguration config = xeus::load_configuration(file_name);

    xeus::xkernel kernel(config, xeus::get_user_name(), std::move(context),
                         std::move(interpreter), xeus::make_xserver_zmq);

    std::clog << "Starting xeus-clang-repl kernel...\n\n"
                 "If you want to connect to this kernel from an other client, "
                 "you can use"
                 " the " +
                     file_name + " file."
              << std::endl;

    kernel.start();
  } else {
    xeus::xkernel kernel(xeus::get_user_name(), std::move(context),
                         std::move(interpreter), xeus::make_xserver_zmq);

    const auto &config = kernel.get_config();
    std::clog << "Starting xeus-clang-repl kernel...\n\n"
                 "If you want to connect to this kernel from an other client, "
                 "just copy"
                 " and paste the following content inside of a `kernel.json` "
                 "file. And then run for example:\n\n"
                 "# jupyter console --existing kernel.json\n\n"
                 "kernel.json\n```\n{\n"
                 "    \"transport\": \"" +
                     config.m_transport +
                     "\",\n"
                     "    \"ip\": \"" +
                     config.m_ip +
                     "\",\n"
                     "    \"control_port\": " +
                     config.m_control_port +
                     ",\n"
                     "    \"shell_port\": " +
                     config.m_shell_port +
                     ",\n"
                     "    \"stdin_port\": " +
                     config.m_stdin_port +
                     ",\n"
                     "    \"iopub_port\": " +
                     config.m_iopub_port +
                     ",\n"
                     "    \"hb_port\": " +
                     config.m_hb_port +
                     ",\n"
                     "    \"signature_scheme\": \"" +
                     config.m_signature_scheme +
                     "\",\n"
                     "    \"key\": \"" +
                     config.m_key +
                     "\"\n"
                     "}\n```\n";

    kernel.start();
  }

  return 0;
}