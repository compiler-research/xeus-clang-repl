#include <cstddef>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Python.h"
#include "clang/Frontend/CompilerInstance.h"
#include "llvm/Support/DynamicLibrary.h"
#include "llvm/ExecutionEngine/Orc/LLJIT.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/Casting.h"

#include "xeus-clang-repl/xoptions.hpp"

#include "xeus-clang-repl/xparser.hpp"

#include "pythonexec.hpp"

namespace xcpp
{
    static PyObject *gMainDict = 0;
    void pythonexec::startup()
    {
        Py_Initialize();
        gMainDict = PyModule_GetDict(PyImport_AddModule(("__main__")));
        Py_INCREF(gMainDict);
        if (!gMainDict)
            printf("Could not add module __main__"); 
    }

    xoptions pythonexec::get_options()
    {
        xoptions options{"python", "Start executing Python Cell"};
        return options;
    }

    void pythonexec::execute(std::string& line, std::string& cell)
    {
        // std::istringstream iss(line);
        // std::vector<std::string> results((std::istream_iterator<std::string>(iss)),
        //                          std::istream_iterator<std::string>());
        startup();
        auto options = get_options();
        auto result = options.parse(line);

        std::string code;

        code += cell;
        if (trim(code).empty()) return;
        PyRun_SimpleString(code.c_str());
    }
}
