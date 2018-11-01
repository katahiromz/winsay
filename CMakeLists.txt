# CMakeLists.txt --- CMake project settings
#    ex) cmake -G "Visual Studio 9 2008"
#    ex) cmake -DCMAKE_BUILD_TYPE=Release -G "MSYS Makefiles"
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 2.4)

# project name and language
project(winsay CXX)

# check build type
if (NOT CMAKE_BUILD_TYPE)
    message(STATUS "No build type selected, default to Debug")
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# make uppercase string of build type
string(TOUPPER "${CMAKE_BUILD_TYPE}" uppercase_CMAKE_BUILD_TYPE)

# check build type again
if (NOT uppercase_CMAKE_BUILD_TYPE MATCHES "^(DEBUG|RELEASE|RELWITHDEBINFO|MINSIZEREL)$")
    message(FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

# set output directory (build/)
#set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})
#set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# enable some languages
enable_language(C)
#enable_language(RC)

if (CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    # using Clang
    set(CMAKE_C_FLAGS "-static")
    set(CMAKE_CXX_FLAGS "-static")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    # using GCC
    set(CMAKE_C_FLAGS "-static -static-libgcc -static-libstdc++")
    set(CMAKE_CXX_FLAGS "-static -static-libgcc -static-libstdc++")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
    # replace "/MD" with "/MT" (building without runtime DLLs)
    set(CompilerFlags
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    foreach(CompilerFlags ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlags} "${${CompilerFlags}}")
    endforeach()
endif()

##############################################################################

# USE_GETOPT_PORT option
option(USE_GETOPT_PORT "Use portable getopt" OFF)

# check getopt_long function
include(CheckIncludeFile)
include(CheckSymbolExists)
check_include_file("getopt.h" HAVE_GETOPT_H)
check_symbol_exists("getopt_long" "getopt.h" HAVE_GETOPT_LONG)

if (HAVE_GETOPT_H)
    add_definitions(-DHAVE_GETOPT_H=1)
endif()
if (HAVE_GETOPT_LONG)
    add_definitions(-DHAVE_GETOPT_LONG=1)
endif()

if (HAVE_GETOPT_LONG AND NOT USE_GETOPT_PORT)
    # we don't use getopt_port
    message(STATUS "We don't use getopt_port")

    # executable
    add_executable(winsay-bin winsay.cpp)
    set_target_properties(winsay-bin PROPERTIES OUTPUT_NAME winsay)
    target_link_libraries(winsay-bin ole32)

    # library
    add_library(winsay STATIC winsay.cpp)
    target_compile_definitions(winsay PRIVATE -DWINSAY_LIBRARY)
else()
    # we use getopt_port
    add_definitions(-DUSE_GETOPT_PORT=1)
    message(STATUS "We use getopt_port")
    include_directories(getopt_port)

    # executable
    add_executable(winsay-bin winsay.cpp getopt_port/getopt.c)
    set_target_properties(winsay-bin PROPERTIES OUTPUT_NAME winsay)
    target_link_libraries(winsay-bin ole32)

    # library
    add_library(winsay STATIC winsay.cpp getopt_port/getopt.c)
    target_compile_definitions(winsay PRIVATE -DWINSAY_LIBRARY)
endif()

##############################################################################
