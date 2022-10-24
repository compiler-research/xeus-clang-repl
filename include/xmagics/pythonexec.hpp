/***********************************************************************************
* Copyright (c) 2016, Johan Mabille, Loic Gouarin, Sylvain Corlay, Wolf Vollprecht *
* Copyright (c) 2016, QuantStack                                                   *
*                                                                                  *
* Distributed under the terms of the BSD 3-Clause License.                         *
*                                                                                  *
* The full license is in the file LICENSE, distributed with this software.         *
************************************************************************************/

#ifndef XMAGICS_PYTHONEXEC_HPP
#define XMAGICS_PYTHONEXEC_HPP

#include <cstddef>
#include <string>

#include "xeus-clang-repl/xbuffer.hpp"
#include "xeus-clang-repl/xmagics.hpp"
#include "xeus-clang-repl/xoptions.hpp"
#include "xeus-clang-repl/xinterpreter.hpp"

namespace xcpp
{
    class pythonexec : public xmagic_cell
    {
    public:
    
        virtual void operator()(const std::string& line, const std::string& cell) override
        {
            std::string cline = line;
            std::string ccell = cell;
            execute(cline, ccell);
        };
        static void update_python_dict_var(const char * name, int value);
        static void update_python_dict_var_vector(const char *name, std::vector<int> &data);
        void check_python_globals();
        void exec_python_simple_command(const std::string code);
        static std::string transfer_python_ints_utility();
        static std::string transfer_python_lists_utility();
        static bool python_check_for_initialisation();

    private:

        static void startup();
        xoptions get_options();
        void execute(std::string& line, std::string& cell);
    };
}
#endif
