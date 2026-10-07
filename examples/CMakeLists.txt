cmake_minimum_required(VERSION 3.21)
project(whitaker_examples LANGUAGES C CXX)

find_package(Whitaker CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_executable(whitaker-c-example c/main.c)
target_compile_features(whitaker-c-example PRIVATE c_std_11)
target_link_libraries(whitaker-c-example PRIVATE Whitaker::whitaker)

add_executable(whitaker-cpp-example cpp/main.cpp)
target_compile_features(whitaker-cpp-example PRIVATE cxx_std_17)
target_link_libraries(whitaker-cpp-example PRIVATE Whitaker::whitaker
    Threads::Threads)
