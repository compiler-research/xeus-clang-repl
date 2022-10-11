//===---------- xmime.hpp - Input redirection of cin -----------*- C++ -*-===//
//
// Licensed under the Apache License v2.0.
// SPDX-License-Identifier: Apache-2.0
//
// The full license is in the file LICENSE, distributed with this software.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the mime interface between clang-repl and Jupyter.
//
//===----------------------------------------------------------------------===//

#ifndef XCPP_MIME_HPP
#define XCPP_MIME_HPP

#include "clang/Frontend/CompilerInstance.h"

#include "nlohmann/json.hpp"

#include <complex>
#include <sstream>

namespace nl = nlohmann;

namespace xcpp {

namespace detail {

// Generic mime_bundle_repr() implementation
// via std::ostringstream.
template <class T> nl::json mime_bundle_repr_via_sstream(const T &value) {
  auto bundle = nl::json::object();

  std::ostringstream oss;
  oss << value;

  bundle["text/plain"] = oss.str();
  return bundle;
}

} // namespace detail

// Default implementation of mime_bundle_repr
template <class T> nl::json mime_bundle_repr(const T &value) {
  auto bundle = nl::json::object();
  bundle["text/plain"] = clang::printValue(&value);
  return bundle;
}

// Implementation for std::complex.
template <class T> nl::json mime_bundle_repr(const std::complex<T> &value) {
  return detail::mime_bundle_repr_via_sstream(value);
}

// Implementation for long double. This is a workaround for
// https://github.com/jupyter-xeus/xeus-cling/issues/220
inline nl::json mime_bundle_repr(const long double &value) {
  return detail::mime_bundle_repr_via_sstream(value);
}
} // namespace xcpp

#endif
