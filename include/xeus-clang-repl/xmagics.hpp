//===--------- xmagics.hpp - Support for Jupyter magics ---------*- C++ -*-===//
//
// Licensed under the Apache License v2.0.
// SPDX-License-Identifier: Apache-2.0
//
// The full license is in the file LICENSE, distributed with this software.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the Jupyter magics interface.
//
//===----------------------------------------------------------------------===//

#ifndef XCPP_MAGICS_HPP
#define XCPP_MAGICS_HPP

#include <map>
#include <memory>

#include "xeus-clang-repl/xoptions.hpp"
#include "xeus-clang-repl/xpreamble.hpp"

namespace xcpp {
enum struct xmagic_type { cell, line };

struct xmagic_line {
  virtual void operator()(const std::string &line) = 0;
};

struct xmagic_cell {
  virtual void operator()(const std::string &line, const std::string &cell) = 0;
};

struct xmagic_line_cell : public xmagic_line, xmagic_cell {};
} // namespace xcpp
#endif
