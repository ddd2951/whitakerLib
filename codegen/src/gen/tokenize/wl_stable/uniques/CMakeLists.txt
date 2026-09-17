add_library(gen_tokenize_uniques STATIC
    form.cpp
    part.cpp
    grammar.cpp
    senses.cpp
    line.cpp
    uniques.cpp
    uniques_post_tests.cpp)
target_link_libraries(gen_tokenize_uniques PUBLIC gen_headers gen_types)
target_link_libraries(gen_tokenize_uniques PRIVATE gen_policy gen_error)
