add_library(gen_construction_wl_stable STATIC construction.cpp)
target_link_libraries(gen_construction_wl_stable PUBLIC gen_headers)
target_link_libraries(gen_construction_wl_stable PRIVATE
    gen_policy
    gen_config
    gen_util
    gen_error)
