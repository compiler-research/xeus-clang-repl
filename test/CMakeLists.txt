####################################################################################
# Copyright (c) 2016, Johan Mabille, Loic Gouarin, Sylvain Corlay, Wolf Vollprecht #
# Copyright (c) 2016, QuantStack                                                   #
#                                                                                  #
# Distributed under the terms of the BSD 3-Clause License.                         #
#                                                                                  #
# The full license is in the file LICENSE, distributed with this software.         #
####################################################################################

# Unit tests
# ==========

cmake_minimum_required(VERSION 3.1)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    project(xeus-clang-repl-test)

    enable_testing()

    find_package(xeus-clang-repl REQUIRED CONFIG)
endif ()

message(STATUS "Forcing tests build type to Release")
set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)

string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

if(CMAKE_CXX_COMPILER_ID MATCHES MSVC)
    add_compile_options(/EHsc /MP /bigobj)
    set(CMAKE_EXE_LINKER_FLAGS /MANIFEST:NO)
endif()

find_package(Threads)

set(XEUS_CLANG_REPL_TESTS
    test_stream.cpp
)

add_executable(test_xeus_clang_repl ${XEUS_CLANG_REPL_TESTS})

if (APPLE)
    set_target_properties(test_xeus_clang_repl PROPERTIES
        MACOSX_RPATH ON
    )
else()
    set_target_properties(test_xeus_clang_repl PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        SKIP_BUILD_RPATH FALSE
    )
endif()

set_target_properties(test_xeus_clang_repl PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

target_link_libraries(test_xeus_clang_repl
                      PRIVATE ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(test_xeus_clang_repl PRIVATE ${XEUS_CLANG_REPL_INCLUDE_DIR})

add_custom_target(xtest COMMAND test_xeus_clang_repl DEPENDS test_xeus_clang_repl)
