//===----------- xdisplay.hpp - Display mime bundles ------------*- C++ -*-===//
//
// Licensed under the Apache License v2.0.
// SPDX-License-Identifier: Apache-2.0
//
// The full license is in the file LICENSE, distributed with this software.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the display interface responsible for visualizing mime
//  bundles in the Jupyter frontend.
//
//===----------------------------------------------------------------------===//

#ifndef XCPP_DISPLAY_HPP
#define XCPP_DISPLAY_HPP

#include "xmime.hpp"

#include "nlohmann/json.hpp"

namespace nl = nlohmann;

namespace xcpp {
template <class T> void display(const T &t) {
  using ::xcpp::mime_bundle_repr;
  xeus::get_interpreter().display_data(mime_bundle_repr(t), nl::json::object(),
                                       nl::json::object());
}

template <class T>
void display(const T &t, xeus::xguid id, bool update = false) {
  nl::json transient;
  transient["display_id"] = id;
  using ::xcpp::mime_bundle_repr;
  if (update) {
    xeus::get_interpreter().update_display_data(
        mime_bundle_repr(t), nl::json::object(), std::move(transient));
  } else {
    xeus::get_interpreter().display_data(
        mime_bundle_repr(t), nl::json::object(), std::move(transient));
  }
}

inline void clear_output(bool wait = false) {
  xeus::get_interpreter().clear_output(wait);
}
} // namespace xcpp

#endif
