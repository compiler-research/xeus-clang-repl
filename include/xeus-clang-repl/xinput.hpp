//===---------- xinput.hpp - Input redirection of cin -----------*- C++ -*-===//
//
// Licensed under the Apache License v2.0.
// SPDX-License-Identifier: Apache-2.0
//
// The full license is in the file LICENSE, distributed with this software.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the xinput interface responsible for redirecting std::cin
//  to the Jupyter frontend.
//
//===----------------------------------------------------------------------===//

#ifndef XCPP_INPUT_HPP
#define XCPP_INPUT_HPP

#include "xeus-clang-repl/xbuffer.hpp"

#include <streambuf>

namespace xcpp {
/// Input_redirection is a scope guard implementing the redirection of
/// std::cin() to the frontend through an input_request message.
class input_redirection {
public:
  input_redirection(bool allow_stdin);
  ~input_redirection();

private:
  std::streambuf *p_cin_strbuf;
  xinput_buffer m_cin_buffer;
};
} // namespace xcpp

#endif
