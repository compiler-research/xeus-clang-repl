//===-------------- xoptions.hpp - Option Parsing ---------------*- C++ -*-===//
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

#ifndef XCPP_OPTIONS_HPP
#define XCPP_OPTIONS_HPP

#include <string>

#include "cxxopts.hpp"

namespace xcpp {
struct xoptions : public cxxopts::Options {
  using parent = cxxopts::Options;
  using parent::Options;
  cxxopts::ParseResult parse(const std::string &line);
};
} // namespace xcpp
#endif
