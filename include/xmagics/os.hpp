//===----------------- os.hpp - Option Parsing ------------------*- C++ -*-===//
//
// Licensed under the Apache License v2.0.
// SPDX-License-Identifier: Apache-2.0
//
// The full license is in the file LICENSE, distributed with this software.
//
//===----------------------------------------------------------------------===//
//
// This file is does option parsing in Jupyter cells based on cxxopts.
//
//===----------------------------------------------------------------------===//

#ifndef XMAGICS_OS_HPP
#define XMAGICS_OS_HPP

#include "xeus-clang-repl/xmagics.hpp"
#include "xeus-clang-repl/xoptions.hpp"

#include <string>

namespace xcpp {
class writefile : public xmagic_cell {
public:
  xoptions get_options();
  virtual void operator()(const std::string &line,
                          const std::string &cell) override;

private:
  static bool is_file_exist(const char *fileName);
};
} // namespace xcpp
#endif
