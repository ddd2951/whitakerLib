add_library(gen_source STATIC source.cpp)
target_link_libraries(gen_source PUBLIC gen_headers)
target_link_libraries(gen_source PRIVATE
    gen_policy
    gen_config
    gen_util
    gen_error)
