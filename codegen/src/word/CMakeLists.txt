add_library(gen_word STATIC word.cpp)
target_link_libraries(gen_word PUBLIC gen_headers gen_source)
target_link_libraries(gen_word PRIVATE
    gen_policy
    gen_config
    gen_error)
