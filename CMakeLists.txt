####################################################################################
# Copyright (c) 2016, Johan Mabille, Loic Gouarin, Sylvain Corlay, Wolf Vollprecht #
# Copyright (c) 2016, QuantStack                                                   #
#                                                                                  #
# Distributed under the terms of the BSD 3-Clause License.                         #
#                                                                                  #
# The full license is in the file LICENSE, distributed with this software.         #
####################################################################################

cmake_minimum_required(VERSION 3.4.3)
project(xeus-clang-repl)

set(XEUS_CLANG-REPL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Versionning
# ===========

file(STRINGS "${XEUS_CLANG-REPL_INCLUDE_DIR}/xeus-clang-repl/xeus_clang-repl_config.hpp" xeus_clang_repl_version_defines
     REGEX "#define XEUS_CLANG_REPL_VERSION_(MAJOR|MINOR|PATCH)")
foreach(ver ${xeus_clang_repl_version_defines})
    if(ver MATCHES "#define XEUS_CLANG_REPL_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(XEUS_CLANG_REPL_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()
set(${PROJECT_NAME}_VERSION
    ${XEUS_CLANG_REPL_VERSION_MAJOR}.${XEUS_CLANG_REPL_VERSION_MINOR}.${XEUS_CLANG_REPL_VERSION_PATCH})
message(STATUS "Building xeus-clang-repl v${${PROJECT_NAME}_VERSION}")

# Configuration
# =============

include(GNUInstallDirs)

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xcpp11/kernel.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/share/jupyter/kernels/xcpp11/kernel.json"
)

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xcpp14/kernel.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/share/jupyter/kernels/xcpp14/kernel.json"
)

configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xcpp17/kernel.json.in"
    "${CMAKE_CURRENT_BINARY_DIR}/share/jupyter/kernels/xcpp17/kernel.json"
)

################
# Dependencies #
################

set(xeus_REQUIRED_VERSION 1.0.0)
set(cppzmq_REQUIRED_VERSION 4.3.0)

find_package(xeus ${xeus_REQUIRED_VERSION} REQUIRED)
find_package(cppzmq ${cppzmq_REQUIRED_VERSION} REQUIRED)
find_package(pugixml REQUIRED)

#find_package(LLVM REQUIRED CONFIG PATHS "${LLVM_CONFIG_EXTRA_PATH_HINTS}" NO_DEFAULT_PATH)
#find_package(Clang REQUIRED CONFIG PATHS "${LLVM_CONFIG_EXTRA_PATH_HINTS}/lib/cmake/clang/" NO_DEFAULT_PATH)
find_package(LLVM "15" REQUIRED CONFIG PATHS "${LLVM_CONFIG_EXTRA_PATH_HINTS}")
find_package(Clang REQUIRED CONFIG PATHS "${LLVM_BINARY_DIR}/lib/cmake/clang/" "${LLVM_CONFIG_EXTRA_PATH_HINTS}/lib/cmake/clang/" NO_DEFAULT_PATH)
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIR})
include(AddLLVM)

add_definitions(-DCLANG_RESOURCE_DIR="${LLVM_BINARY_DIR}/lib/clang/${LLVM_PACKAGE_VERSION}")

find_package(cxxopts REQUIRED)
find_package(PythonLibs REQUIRED)

#########
# flags #
#########

include(HandleLLVMOptions)
include(CheckCXXCompilerFlag)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
        "Choose the build type, e.g. Release or Debug." FORCE)
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4141")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4267 /wd4715 /wd4146 /wd4129")
endif()

OPTION(DISABLE_ARCH_NATIVE "disable -march=native flag" OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-parameter -Wextra -Wreorder")
    if (DISABLE_ARCH_NATIVE)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif()

    CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP14_FLAG)

    if (HAS_CPP14_FLAG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    else()
        message(FATAL_ERROR "Unsupported compiler -- xeus requires C++14 support!")
    endif()
endif()

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

###################
# Target and link #
###################

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib; ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

# xeus-clang-repl sources
set(XEUS_CLANG_REPL_SRC
    src/xinput.cpp
    src/xinterpreter.cpp
    src/xoptions.cpp
    src/xparser.cpp
    src/xholder_clang-repl.cpp
    src/xmagics/os.cpp
    src/xmagics/pythonexec.cpp
    
    
)

# xeus-clang-repl headers
set(XEUS_CLANG_REPL_HEADERS
    include/xeus-clang-repl/xinput.hpp
    include/xeus-clang-repl/xbuffer.hpp
    include/xeus-clang-repl/xeus_clang-repl_config.hpp
    include/xeus-clang-repl/xholder_clang-repl.hpp
    include/xeus-clang-repl/xinterpreter.hpp
    include/xeus-clang-repl/xmagics.hpp
    include/xeus-clang-repl/xmanager.hpp
    include/xeus-clang-repl/xoptions.hpp
    include/xeus-clang-repl/xpreamble.hpp
    include/xeus-clang-repl/xparser.hpp
    include/xmagics/os.hpp
    include/xmagics/pythonexec.hpp
)

# xcpp sources
set(XCPP_SRC
    src/main.cpp
)

# xcpp headers (needed at runtime by the C++ kernel)
set(XCPP_HEADERS
    include/xcpp/xmime.hpp
    include/xcpp/xdisplay.hpp
)

# xeus-clang-repl is the target for the library
set(LLVM_REQUIRES_EH ON)
set(LLVM_REQUIRES_RTTI ON)
llvm_add_library(xeus-clang-repl SHARED ${XEUS_CLANG_REPL_SRC} ${XEUS_CLANG_REPL_HEADERS})

# xcpp is the target for the kernel executable
add_executable(xcpp ${XCPP_SRC})
set_target_properties(xcpp PROPERTIES ENABLE_EXPORTS 1)
target_link_libraries(xcpp PUBLIC xeus-clang-repl pthread)

set(LLVM_NO_DEAD_STRIP 1)

if (APPLE)
    set_target_properties(xcpp xeus-clang-repl PROPERTIES
        MACOSX_RPATH ON
    )
else()
    set_target_properties(xcpp xeus-clang-repl PROPERTIES
        BUILD_WITH_INSTALL_RPATH 1
        SKIP_BUILD_RPATH FALSE
    )
endif()

set_target_properties(xcpp xeus-clang-repl PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
)

set( LLVM_LINK_COMPONENTS
  ${LLVM_TARGETS_TO_BUILD}
  Core
  LineEditor
  Option
  OrcJIT
  Support
  )

target_include_directories(xeus-clang-repl SYSTEM
                           PUBLIC
                           $<BUILD_INTERFACE:${XEUS_CLANG-REPL_INCLUDE_DIR}>
                           $<INSTALL_INTERFACE:include>)
target_link_libraries(xeus-clang-repl PUBLIC clangInterpreter xeus pugixml cxxopts::cxxopts)

target_include_directories(xeus-clang-repl PUBLIC ${PYTHON_INCLUDE_DIRS})
target_link_libraries(xeus-clang-repl PUBLIC ${PYTHON_LIBRARIES})
target_link_libraries(xeus-clang-repl ${PYTHON_LIBRARIES_Development_Main})

set_target_properties(xeus-clang-repl PROPERTIES
                      PUBLIC_HEADER "${XEUS_CLANG_REPL_HEADERS}"
                      COMPILE_DEFINITIONS "XEUS_CLANG_REPL_EXPORTS"
                      PREFIX ""
                      VERSION ${${PROJECT_NAME}_VERSION}
                      SOVERSION ${XEUS_CLANG_REPL_VERSION_MAJOR}
                      OUTPUT_NAME "libxeus-clang-repl")

if(MSVC)
    set_target_properties(xeus-clang-repl PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS 1)  # Internal string

    # Internal string
    set(clang_repl_exports ?kEmptyCollection@valuePrinterInternal@clang_repl@@3QEBDEB)

    # RTTI/C++ symbols
    set(clang_repl_exports ${clang_repl_exports} ??_7type_info@@6B@
        ?__type_info_root_node@@3U__type_info_node@@A
        ?nothrow@std@@3Unothrow_t@1@B
        ?_Facet_Register@std@@YAXPEAV_Facet_base@1@@Z
    )

    # Compiler added symbols for static variables. NOT for VStudio < 2015
    set(clang_repl_exports ${clang_repl_exports} _Init_thread_abort _Init_thread_epoch
        _Init_thread_footer _Init_thread_header _tls_index
    )

    # new/delete variants needed when linking to static msvc runtime (esp. Debug)
    set(clang_repl_exports ${clang_repl_exports}
        #??2@YAPEAX_KPEBDH@Z  ## not used in clang_repl
        #??_U@YAPEAX_KPEBDH@Z ## not used in clang_repl
        ??2@YAPEAX_K@Z
        ??3@YAXPEAX@Z
        ??3@YAXPEAX_K@Z
        ??_U@YAPEAX_K@Z
        ??_V@YAXPEAX@Z
    )

    # Most (if not all) of these MSVC decided are inlines that aren't exported
    # TODO: There are probably some missing. Why?
    set(clang_repl_exports ${clang_repl_exports} ?print@Decl@clang@@QEBAXAEAVraw_ostream@llvm@@I_N@Z
        ??6raw_ostream@llvm@@QEAAAEAV01@PEBX@Z
        ?decls_begin@DeclContext@clang@@QEBA?AVdecl_iterator@12@XZ
        ?errs@llvm@@YAAEAVraw_ostream@1@XZ
        ?grow_pod@SmallVectorBase@llvm@@IEAAXPEAX_K1@Z
        ?write@raw_ostream@llvm@@QEAAAEAV12@E@Z
        ?write@raw_ostream@llvm@@QEAAAEAV12@PEBD_K@Z
        ?castFromDeclContext@Decl@clang@@SAPEAV12@PEBVDeclContext@2@@Z
        ??1raw_ostream@llvm@@UEAA@XZ
        ??1raw_string_ostream@llvm@@UEAA@XZ
        ?flush_nonempty@raw_ostream@llvm@@AEAAXXZ
        ?getASTContext@Decl@clang@@QEBAAEAVASTContext@2@XZ
        ?handle@raw_ostream@llvm@@EEAAXXZ
        ?preferred_buffer_size@raw_ostream@llvm@@MEBA_KXZ
        ?write_impl@raw_string_ostream@llvm@@EEAAXPEBD_K@Z
        ?castToDeclContext@Decl@clang@@SAPEAVDeclContext@2@PEBV12@@Z
        ?classof@DeclContext@clang@@SA_NPEBVDecl@2@@Z
        ?setValueNoAlloc@internal@runtime@clang@@YAXPEAX00D@Z
        ?setValueNoAlloc@internal@runtime@clang@@YAXPEAX00D_K@Z
        ?setValueNoAlloc@internal@runtime@clang@@YAXPEAX00DM@Z
        ?setValueNoAlloc@internal@runtime@clang@@YAXPEAX00DN@Z
        ?setValueWithAlloc@internal@runtime@clang@@YAPEAXPEAX00D@Z
        ?setValueNoAlloc@internal@runtime@clang@@YAXPEAX00DPEBX@Z
        clang_runtime_internal_throwIfInvalidPointer
    )

    set(clang_repl_exports ${clang_repl_exports}
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEAPEBD@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEAPEBX@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBC@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBD@Z 
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBE@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBF@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBG@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBH@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBI@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBJ@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBK@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBM@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBN@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBO@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBQEBD@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBV23@@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBV?$$basic_string@_SU?$$char_traits@_S@std@@V?$$allocator@_S@2@@3@@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBV?$$basic_string@_UU?$$char_traits@_U@std@@V?$$allocator@_U@2@@3@@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBV?$$basic_string@_WU?$$char_traits@_W@std@@V?$$allocator@_W@2@@3@@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBVValue@1@@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEBX@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEB_J@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEB_K@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEB_N@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEB_S@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEB_U@Z
        ?printValue@clang@@YA?AV?$$basic_string@DU?$$char_traits@D@std@@V?$$allocator@D@2@@std@@PEB_W@Z
    )

    if($<CONFIG:Debug>)
        set(clang_repl_exports ${clang_repl_exports} ??$dyn_cast@VValueDecl@clang@@$$CBVDecl@2@@llvm@@YAPEBVValueDecl@clang@@PEBVDecl@2@@Z)
    endif()

    # List to '/EXPORT:sym0 /EXPORT:sym1 /EXPORT:sym2 ...'
    foreach(sym ${clang_repl_exports})
        set(clang_repl_link_str "${clang_repl_link_str} /EXPORT:${sym}")
    endforeach(sym ${clang_repl_exports})

    set_property(TARGET xeus-clang-repl APPEND_STRING PROPERTY LINK_FLAGS ${clang_repl_link_str})
endif(MSVC)

#########
# Tests #
#########

#OPTION(BUILD_TESTS "xeus-clang-repl test suite" OFF)
#OPTION(DOWNLOAD_GTEST "build gtest from downloaded sources" OFF)

#if(DOWNLOAD_GTEST OR GTEST_SRC_DIR)
#    set(BUILD_TESTS ON)
#endif()

#if(BUILD_TESTS)
#    add_subdirectory(test)
#endif()

################
# Installation #
################

include(CMakePackageConfigHelpers)

set(XEUS_CLANG_REPL_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE STRING "install path for xeus-clang-replConfig.cmake")

# Install xeus-clang-repl and xcpp
install(TARGETS xeus-clang-repl
        EXPORT ${PROJECT_NAME}-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xeus-clang-repl)
install(TARGETS xcpp
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install xcpp headers
install(FILES ${XCPP_HEADERS}
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xcpp)

# Configuration and data directories for jupyter and xeus-clang-repl
set(XJUPYTER_DATA_DIR   "share/jupyter"    CACHE STRING "Jupyter data directory")

# Install Jupyter kernelspecs
set(XCPP_KERNELSPEC_DIR ${CMAKE_CURRENT_BINARY_DIR}/share/jupyter/kernels)
install(DIRECTORY ${XCPP_KERNELSPEC_DIR}
        DESTINATION ${XJUPYTER_DATA_DIR}
        PATTERN "*.in" EXCLUDE)

# Extra path for installing Jupyter kernelspec
if (XEXTRA_JUPYTER_DATA_DIR)
    install(DIRECTORY ${XCPP_KERNELSPEC_DIR}
            DESTINATION ${XEXTRA_JUPYTER_DATA_DIR}
            PATTERN "*.in" EXCLUDE)
endif(XEXTRA_JUPYTER_DATA_DIR)

# Makes the project importable from the build directory
export(EXPORT ${PROJECT_NAME}-targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")

# Configure 'xeus-clang-replConfig.cmake' for a build tree
set(XEUS_CLANG_REPL_CONFIG_CODE "####### Expanded from \@XEUS_CLANG_REPL_CONFIG_CODE\@ #######\n")
set(XEUS_CLANG_REPL_CONFIG_CODE "${XEUS_CLANG_REPL_CONFIG_CODE}set(CMAKE_MODULE_PATH \"${CMAKE_CURRENT_SOURCE_DIR}/cmake;\${CMAKE_MODULE_PATH}\")\n")
set(XEUS_CLANG_REPL_CONFIG_CODE "${XEUS_CLANG_REPL_CONFIG_CODE}##################################################")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${PROJECT_BINARY_DIR})

# Configure 'xeus-clang-replConfig.cmake' for an install tree
set(XEUS_CLANG_REPL_CONFIG_CODE "")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${XEUS_CLANG_REPL_CMAKECONFIG_INSTALL_DIR})

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${${PROJECT_NAME}_VERSION}
                                 COMPATIBILITY AnyNewerVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              DESTINATION ${XEUS_CLANG_REPL_CMAKECONFIG_INSTALL_DIR})
install(EXPORT ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${XEUS_CLANG_REPL_CMAKECONFIG_INSTALL_DIR})
