cmake_minimum_required(VERSION 3.20)
project(whitaker_examples LANGUAGES C CXX)

find_package(Whitaker CONFIG REQUIRED)

# No version floor before 1.0; find_package reports what it found.
message(STATUS "Examples build against Whitaker ${Whitaker_VERSION}")

add_executable(whitaker-c-example c/main.c)
target_link_libraries(whitaker-c-example PRIVATE Whitaker::whitaker)

add_executable(whitaker-cpp-example cpp/main.cpp)
target_link_libraries(whitaker-cpp-example PRIVATE Whitaker::whitaker)
target_compile_features(whitaker-cpp-example PRIVATE cxx_std_17)
