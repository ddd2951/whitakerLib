add_library(gen_tokenize_inflects STATIC
    part.cpp
    grammar.cpp
    ending.cpp
    line.cpp
    inflects.cpp
    inflects_post_tests.cpp)
target_link_libraries(gen_tokenize_inflects PUBLIC gen_headers gen_types)
target_link_libraries(gen_tokenize_inflects PRIVATE gen_policy gen_error)
