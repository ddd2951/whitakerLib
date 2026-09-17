add_subdirectory(dictline)
add_subdirectory(inflects)
add_subdirectory(uniques)
add_subdirectory(addons)

add_library(gen_tokenize_wl_stable STATIC tokenize.cpp common/pre_tests.cpp)
target_link_libraries(gen_tokenize_wl_stable PUBLIC gen_headers gen_types)
target_link_libraries(gen_tokenize_wl_stable PRIVATE
    gen_policy
    gen_error
    gen_tokenize_dictline
    gen_tokenize_inflects
    gen_tokenize_uniques
    gen_tokenize_addons)
