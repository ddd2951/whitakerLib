cmake_minimum_required(VERSION 3.28)
project(oracle LANGUAGES CXX C)

set(WHITAKERLIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/.." CACHE PATH "whitakerLib source tree to test")
get_filename_component(WHITAKERLIB_DIR "${WHITAKERLIB_DIR}" ABSOLUTE)
set(WHITAKER_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(WHITAKER_INSTALL OFF CACHE BOOL "" FORCE)
set(WHITAKER_WERROR OFF CACHE BOOL "" FORCE)
add_subdirectory(${WHITAKERLIB_DIR} whitakerLib EXCLUDE_FROM_ALL)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(ADA_DATA_DIR "" CACHE PATH "WORDS' data directory")
if(NOT ADA_DATA_DIR)
  message(FATAL_ERROR "set -DADA_DATA_DIR=/path/to/words/data")
endif()
add_executable(lines
    src/main.cpp
    src/ada_lines.cpp
    src/library_lines.cpp
    src/compare.cpp
    src/captures.cpp
    src/show.cpp
    src/probe.cpp
    src/report.cpp
    src/dictionary.cpp
    src/ada_parse.cpp)
set_target_properties(lines PROPERTIES CXX_STANDARD 26 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_compile_options(lines PRIVATE -freflection -Wall -Wextra)
target_compile_definitions(lines PRIVATE WHITAKERLIB_DIR="${WHITAKERLIB_DIR}" ADA_DATA_DIR="${ADA_DATA_DIR}")
target_link_libraries(lines PRIVATE Whitaker::whitaker)
