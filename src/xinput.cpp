//===----------- xinput.cpp - Handles various input redirection -----------===//
//
// Licensed under the Apache License v2.0.
// SPDX-License-Identifier: Apache-2.0
//
// The full license is in the file LICENSE, distributed with this software.
//
//===----------------------------------------------------------------------===//
//
// This file is responsible for redirecting input of std::cin to the Jupyter
// frontend.
//
//===----------------------------------------------------------------------===//

#include "xeus-clang-repl/xinput.hpp"

#include "xeus/xinput.hpp"
#include "xeus/xinterpreter.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace xcpp {
void notimplemented(const std::string &) {
  throw std::runtime_error("This frontend does not support input requests");
}

/***************************************
 * Implementation of input_redirection *
 ***************************************/

input_redirection::input_redirection(bool allow_stdin)
    : p_cin_strbuf(std::cin.rdbuf()),
      m_cin_buffer(allow_stdin ? xinput_buffer([](std::string &value) {
        value = xeus::blocking_input_request("", false);
      })
                               : xinput_buffer(notimplemented)) {
  std::cin.rdbuf(&m_cin_buffer);
}

input_redirection::~input_redirection() { std::cin.rdbuf(p_cin_strbuf); }
} // namespace xcpp
