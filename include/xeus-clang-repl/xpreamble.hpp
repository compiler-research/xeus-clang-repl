//===------------ xpreamble.hpp - Preamble support --------------*- C++ -*-===//
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

#ifndef XCPP_PREAMBLE_HPP
#define XCPP_PREAMBLE_HPP

#include "nlohmann/json.hpp"

#include <regex>
#include <string>

namespace nl = nlohmann;

namespace xcpp {
struct xpreamble {
  std::regex pattern;

  bool is_match(const std::string &s) const {
    std::smatch match;
    return std::regex_search(s, match, pattern);
  }

  virtual void apply(const std::string &s, nl::json &kernel_res) = 0;
  virtual xpreamble *clone() const = 0;
  virtual ~xpreamble(){};
};
} // namespace xcpp
#endif
