add_library(gen_error STATIC error.cpp)
target_link_libraries(gen_error PUBLIC gen_headers)
target_link_libraries(gen_error PRIVATE gen_policy)
