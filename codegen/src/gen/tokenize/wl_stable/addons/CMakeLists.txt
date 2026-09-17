add_library(gen_tokenize_addons STATIC
    fix.cpp
    grammar.cpp
    meaning.cpp
    line.cpp
    addons.cpp
    addons_post_tests.cpp)
target_link_libraries(gen_tokenize_addons PUBLIC gen_headers gen_types)
target_link_libraries(gen_tokenize_addons PRIVATE gen_policy gen_error)
