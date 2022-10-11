//===--------------- xparser.hpp - Parsing Support --------------*- C++ -*-===//
//
// Licensed under the Apache License v2.0.
// SPDX-License-Identifier: Apache-2.0
//
// The full license is in the file LICENSE, distributed with this software.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the inteface for parsing, trimming and options parsing in
//  Jupyter cells.
//
//===----------------------------------------------------------------------===//

#ifndef XCPP_PARSER_HPP
#define XCPP_PARSER_HPP

#include <map>
#include <string>
#include <vector>

namespace xcpp {
std::vector<std::string> split_line(const std::string &input,
                                    const std::string &delims,
                                    std::size_t cursor_pos);

std::vector<std::string> get_lines(const std::string &input);

std::vector<std::string> split_from_includes(const std::string &input);

bool short_has_arg(const std::string &opt, const std::string &short_opts);

std::map<std::string, std::string> getopt(std::string &input,
                                          const std::string &short_opts);

std::string trim(std::string const &str);

std::map<std::string, std::string> parse_opts(std::string &line,
                                              const std::string &opts);
} // namespace xcpp
#endif
