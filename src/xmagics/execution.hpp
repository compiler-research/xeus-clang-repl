/***********************************************************************************
 * Copyright (c) 2016, Johan Mabille, Loic Gouarin, Sylvain Corlay, Wolf
 *Vollprecht * Copyright (c) 2016, QuantStack *
 *                                                                                  *
 * Distributed under the terms of the BSD 3-Clause License. *
 *                                                                                  *
 * The full license is in the file LICENSE, distributed with this software. *
 ************************************************************************************/

#ifndef XMAGICS_EXECUTION_HPP
#define XMAGICS_EXECUTION_HPP

#include <cstddef>
#include <string>

#include "cling/Interpreter/Interpreter.h"

#include "xeus-clang-repl/xmagics.hpp"
#include "xeus-clang-repl/xoptions.hpp"

namespace xcpp {
class timeit : public xmagic_line_cell {
public:
  timeit(cling::Interpreter* p);

  virtual void operator()(const std::string& line) override {
    std::string cline = line;
    std::string cell = "";
    execute(cline, cell);
  }

  virtual void operator()(const std::string& line,
                          const std::string& cell) override {
    std::string cline = line;
    std::string ccell = cell;
    execute(cline, ccell);
  }

private:
  cling::Interpreter* m_interpreter;

  xoptions get_options();
  std::string inner(std::size_t number, const std::string& code) const;
  std::string _format_time(double timespan, std::size_t precision) const;
  void execute(std::string& line, std::string& cell);
};
} // namespace xcpp
#endif
