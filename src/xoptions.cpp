//===------------------ xoptions.cpp - Option Parsing ---------------------===//
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

#include "xeus-clang-repl/xoptions.hpp"

#include <sstream>
#include <string>
#include <vector>
#include <iterator>

namespace xcpp {
cxxopts::ParseResult xoptions::parse(const std::string &line) {
  std::istringstream iss(line);
  std::vector<std::string> opt_strings(
      (std::istream_iterator<std::string>(iss)),
      std::istream_iterator<std::string>());

  std::vector<const char *> copt_strings;

  for (std::size_t i = 0; i < opt_strings.size(); ++i) {
    copt_strings.push_back(opt_strings[i].c_str());
  }

  int argc = copt_strings.size();

  // Const-casting as cxxopts::parse moved from (int&, const char**&) to
  // (int&, char**&) between 2.1.0 and 2.1.1.).
  // This should not be required in 2.2.0.
  //
  // Macros CXXOPTS__VERSION_[MAJOR/MINOR/PATCH] were only defined
  // after 2.1.1.]
  auto argv = const_cast<char **>(&copt_strings[0]);
  return parent::parse(argc, argv);
}
} // namespace xcpp
