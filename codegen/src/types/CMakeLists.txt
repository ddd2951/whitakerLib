add_library(gen_types STATIC semantic_test.cpp)
target_link_libraries(gen_types PUBLIC gen_headers)
target_link_libraries(gen_types PRIVATE gen_policy)
