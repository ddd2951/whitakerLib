add_library(gen_expand_wl_stable STATIC
    expand.cpp
    common/reading.cpp
    common/rows.cpp
    facts/slice.cpp
    facts/synthetics.cpp
    facts/candidates.cpp
    facts/uniques.cpp
    facts/sorted.cpp
    facts/adverb_corrections.cpp
    facts/forms.cpp
    facts/readings.cpp
    facts/swept.cpp)
target_link_libraries(gen_expand_wl_stable PUBLIC gen_headers gen_types gen_runtime_facts)
target_link_libraries(gen_expand_wl_stable PRIVATE gen_policy gen_error)
