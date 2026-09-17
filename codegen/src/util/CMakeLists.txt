add_library(gen_util STATIC file_load.cpp)
target_link_libraries(gen_util PUBLIC gen_headers)
target_link_libraries(gen_util PRIVATE gen_policy gen_error)
