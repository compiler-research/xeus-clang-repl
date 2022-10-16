/***********************************************************************************
* Copyright (c) 2016, Johan Mabille, Loic Gouarin, Sylvain Corlay, Wolf Vollprecht *
* Copyright (c) 2016, QuantStack                                                   *
*                                                                                  *
* Distributed under the terms of the BSD 3-Clause License.                         *
*                                                                                  *
* The full license is in the file LICENSE, distributed with this software.         *
************************************************************************************/

#ifndef XEUS_CLANG_REPL_CONFIG_HPP
#define XEUS_CLANG_REPL_CONFIG_HPP

// Project version
#define XEUS_CLANG_REPL_VERSION_MAJOR 0
#define XEUS_CLANG_REPL_VERSION_MINOR 13
#define XEUS_CLANG_REPL_VERSION_PATCH 0

// Composing the version string from major, minor and patch
#define XEUS_CLANG_REPL_CONCATENATE(A, B) XEUS_CLANG_REPL_CONCATENATE_IMPL(A, B)
#define XEUS_CLANG_REPL_CONCATENATE_IMPL(A, B) A##B
#define XEUS_CLANG_REPL_STRINGIFY(a) XEUS_CLANG_REPL_STRINGIFY_IMPL(a)
#define XEUS_CLANG_REPL_STRINGIFY_IMPL(a) #a

#define XEUS_CLANG_REPL_VERSION XEUS_CLANG_REPL_STRINGIFY(XEUS_CLANG_REPL_CONCATENATE(XEUS_CLANG_REPL_VERSION_MAJOR,   \
                       XEUS_CLANG_REPL_CONCATENATE(.,XEUS_CLANG_REPL_CONCATENATE(XEUS_CLANG_REPL_VERSION_MINOR,   \
                                              XEUS_CLANG_REPL_CONCATENATE(.,XEUS_CLANG_REPL_VERSION_PATCH)))))

#ifdef _WIN32
    #ifdef XEUS_CLANG_REPL_EXPORTS
        #define XEUS_CLANG_REPL_API __declspec(dllexport)
    #else
        #define XEUS_CLANG_REPL_API __declspec(dllimport)
    #endif
#else
    #define XEUS_CLANG_REPL_API
#endif

#endif
