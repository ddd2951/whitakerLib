add_library(gen_tokenize_dictline STATIC
    stem.cpp
    part.cpp
    grammar.cpp
    labels.cpp
    senses.cpp
    line.cpp
    dictline.cpp
    dictline_post_tests.cpp)
target_link_libraries(gen_tokenize_dictline PUBLIC gen_headers gen_types)
target_link_libraries(gen_tokenize_dictline PRIVATE gen_policy gen_error)
