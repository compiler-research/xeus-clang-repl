//===--------- xholder_clang-repl.hpp - Preamble holder ---------*- C++ -*-===//
//
// Licensed under the Apache License v2.0.
// SPDX-License-Identifier: Apache-2.0
//
// The full license is in the file LICENSE, distributed with this software.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the preable holder interface.
//
//===----------------------------------------------------------------------===//

#ifndef XCPP_HOLDER_CLANG_REPL_HPP
#define XCPP_HOLDER_CLANG_REPL_HPP

#include "xeus-clang-repl/xpreamble.hpp"

#include "nlohmann/json.hpp"

#include <regex>

namespace nl = nlohmann;

namespace xcpp {
class xholder_preamble {
public:
  xholder_preamble();
  ~xholder_preamble();
  xholder_preamble(const xholder_preamble &rhs);
  xholder_preamble(xholder_preamble &&rhs);
  xholder_preamble(xpreamble *holder);

  xholder_preamble &operator=(const xholder_preamble &rhs);
  xholder_preamble &operator=(xholder_preamble &&rhs);

  xholder_preamble &operator=(xpreamble *holder);

  void swap(xholder_preamble &rhs) { std::swap(p_holder, rhs.p_holder); }

  void apply(const std::string &s, nl::json &kernel_res);
  bool is_match(const std::string &s) const;

  template <class D> D &get_cast() { return dynamic_cast<D &>(*p_holder); }

private:
  xpreamble *p_holder;
};
} // namespace xcpp
#endif
