cmake_minimum_required(VERSION 3.28)
project(gen LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(WHITAKER_SUBTREE ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(${WHITAKER_SUBTREE}/warnings.cmake)

# -Wno-shadow: every `template for` expansion trips -Wshadow (GCC 16.2.1).
set(GEN_OPTIONS -fno-exceptions -fno-rtti -Werror
    ${WHITAKER_WARNINGS} ${WHITAKER_WARNINGS_CXX}
    ${WHITAKER_WARNINGS_GNU} ${WHITAKER_WARNINGS_GNU_CXX} -Wno-shadow)

add_library(gen_runtime STATIC
    ${WHITAKER_SUBTREE}/src/search/relationship_image.cpp
    ${WHITAKER_SUBTREE}/src/search/image_loader.cpp)
target_compile_features(gen_runtime PRIVATE cxx_std_23)
target_compile_options(gen_runtime PRIVATE ${GEN_OPTIONS})
target_include_directories(gen_runtime PUBLIC
    ${WHITAKER_SUBTREE} ${WHITAKER_SUBTREE}/include ${WHITAKER_SUBTREE}/src ${WHITAKER_SUBTREE}/latin)

add_executable(gen_binary
    src/main.cpp
    src/error/error.cpp
    src/util/file_load.cpp
    src/types/latin_test.cpp
    src/readings/format_test.cpp
    src/bindings/python_test.cpp
    src/bindings/c_test.cpp
    src/source/source.cpp
    src/execution_steps/read_dictline.cpp
    src/execution_steps/index_dictline_records.cpp
    src/execution_steps/parse_dictline_records.cpp
    src/execution_steps/add_esse.cpp
    src/execution_steps/read_inflects.cpp
    src/execution_steps/index_inflects_records.cpp
    src/execution_steps/parse_inflects_records.cpp
    src/execution_steps/add_adverb_inflections.cpp
    src/execution_steps/read_uniques.cpp
    src/execution_steps/index_uniques_records.cpp
    src/execution_steps/parse_uniques_records.cpp
    src/execution_steps/read_addons.cpp
    src/execution_steps/index_addons_records.cpp
    src/execution_steps/parse_addons_records.cpp
    src/execution_steps/make_candidates.cpp
    src/execution_steps/prepare_forms.cpp
    src/execution_steps/make_readings.cpp
    src/execution_steps/sweep_readings.cpp
    src/execution_steps/index_results.cpp
    src/execution_steps/classify_addons.cpp
    src/execution_steps/store_addons.cpp
    src/execution_steps/store_dictionary_words.cpp
    src/execution_steps/number_packons.cpp
    src/execution_steps/build_dictionary_classes.cpp
    src/execution_steps/collect_readings.cpp
    src/execution_steps/group_spellings.cpp
    src/execution_steps/order_inflections.cpp
    src/execution_steps/license_dictionary_classes.cpp
    src/execution_steps/license_suffixes.cpp
    src/execution_steps/write_temporary_image.cpp
    src/execution_steps/load_temporary_image.cpp
    src/execution_steps/enumerate_image_spellings.cpp
    src/execution_steps/check_image_readings.cpp
    src/execution_steps/check_dictionary_words.cpp
    src/execution_steps/check_dictionary_word_stems.cpp
    src/execution_steps/check_addon_tables.cpp
    src/execution_steps/publish_image.cpp
    src/execution_steps/encode_image.cpp
    src/execution_steps/prepare_image_destination.cpp
    src/execution_steps/attach_lookup_tables.cpp
    src/execution_steps/build_affix_tables.cpp
    src/execution_steps/assign_rule_labels.cpp
    src/execution_steps/assign_rule_descriptions.cpp
    src/execution_steps/copy_lookup_stems.cpp
    src/execution_steps/group_dictionary_stems.cpp
    src/execution_steps/mark_stem_letters.cpp
    src/execution_steps/compress_lookup_cells.cpp
    src/execution_steps/pack_lookup_rules.cpp
    src/execution_steps/pack_lookup_stems.cpp
    src/execution_steps/build_lookup_heads.cpp
    src/execution_steps/pack_shared_stem_slots.cpp
    src/execution_steps/number_stem_runs.cpp
    src/execution_steps/canonicalize_stem_trie.cpp
    src/execution_steps/build_perfect_suffix_trie.cpp
    src/execution_steps/derive_lookup_bounds.cpp
    src/execution_steps/build_lookup_cells.cpp
    src/execution_steps/build_ending_trie.cpp
    src/execution_steps/build_stem_trie.cpp
    src/execution_steps/number_endings.cpp
    src/execution_steps/group_stem_spellings.cpp
    src/execution_steps/rank_lookup_rules.cpp
    src/execution_steps/sort_lookup_stems.cpp
    src/execution_steps/add_affix_stems.cpp
    src/execution_steps/build_lookup_classes.cpp
    src/execution_steps/measure_candidate_capacity.cpp
    src/execution_steps/collect_perfect_suffixes.cpp
    src/execution_steps/add_unique_rules.cpp
    src/execution_steps/make_lookup_rules.cpp
    src/word/word.cpp
    src/expand/expand.cpp
    src/expand/common/reading.cpp
    src/emitter/image_file.cpp
    src/emitter/relationship_emitter/string_section.cpp)
set_target_properties(gen_binary PROPERTIES OUTPUT_NAME gen_image)
target_include_directories(gen_binary PRIVATE src)
target_compile_features(gen_binary PRIVATE cxx_std_26)
target_compile_options(gen_binary PRIVATE -freflection ${GEN_OPTIONS})
target_link_libraries(gen_binary PRIVATE gen_runtime)

add_custom_target(image COMMAND $<TARGET_FILE:gen_binary> WORKING_DIRECTORY ${WHITAKER_SUBTREE}
    DEPENDS gen_binary USES_TERMINAL)
add_custom_target(gen DEPENDS gen_binary)

# INFO: The Python and C bindings of what whitaker.hpp tags WHITAKER_BIND.
add_executable(bindings src/bindings/main.cpp)
target_include_directories(bindings PRIVATE src)
target_compile_features(bindings PRIVATE cxx_std_26)
target_compile_options(bindings PRIVATE -freflection ${GEN_OPTIONS})
target_link_libraries(bindings PRIVATE gen_runtime)
set_source_files_properties(src/bindings/main.cpp src/bindings/python_test.cpp src/bindings/c_test.cpp
    PROPERTIES COMPILE_OPTIONS -fconstexpr-ops-limit=1000000000)

add_custom_target(write_bindings COMMAND bindings WORKING_DIRECTORY ${WHITAKER_SUBTREE}
    USES_TERMINAL)

# INFO: latin's names and fixed-byte reading shapes, reflected from the declarations.
add_executable(reading_format src/readings/format_main.cpp)
target_include_directories(reading_format PRIVATE src)
target_compile_features(reading_format PRIVATE cxx_std_26)
target_compile_options(reading_format PRIVATE -freflection ${GEN_OPTIONS})
target_link_libraries(reading_format PRIVATE gen_runtime)
set_source_files_properties(src/readings/format_main.cpp src/readings/format_test.cpp
    PROPERTIES COMPILE_OPTIONS -fconstexpr-ops-limit=1000000000)
add_custom_target(write_reading_format COMMAND reading_format WORKING_DIRECTORY ${WHITAKER_SUBTREE}
    DEPENDS reading_format USES_TERMINAL)

get_target_property(IMAGE_SOURCES gen_binary SOURCES)
list(FILTER IMAGE_SOURCES EXCLUDE REGEX "(_test|/main)\\.cpp$")
add_executable(two_ended src/readings/tables_main.cpp ${IMAGE_SOURCES})
target_include_directories(two_ended PRIVATE src)
target_compile_features(two_ended PRIVATE cxx_std_26)
target_compile_options(two_ended PRIVATE -freflection ${GEN_OPTIONS})
target_link_libraries(two_ended PRIVATE gen_runtime)
add_custom_target(write_two_ended COMMAND two_ended --write WORKING_DIRECTORY ${WHITAKER_SUBTREE}
    DEPENDS two_ended USES_TERMINAL)
add_custom_target(check_two_ended COMMAND two_ended --check WORKING_DIRECTORY ${WHITAKER_SUBTREE}
    DEPENDS two_ended USES_TERMINAL)
add_dependencies(gen check_two_ended)
target_sources(gen_binary PRIVATE src/readings/tables_test.cpp)

# INFO: Bounds for the word-local change list, from the same unswept source expansion as the image.
get_target_property(CAPACITY_SOURCES gen_binary SOURCES)
list(FILTER CAPACITY_SOURCES EXCLUDE REGEX "(_test|/main)\\.cpp$")
add_executable(reading_capacity src/readings/capacity_main.cpp ${CAPACITY_SOURCES})
target_include_directories(reading_capacity PRIVATE src)
target_compile_features(reading_capacity PRIVATE cxx_std_26)
target_compile_options(reading_capacity PRIVATE -freflection ${GEN_OPTIONS})
target_link_libraries(reading_capacity PRIVATE gen_runtime)
add_custom_target(write_reading_capacity COMMAND reading_capacity --write WORKING_DIRECTORY ${WHITAKER_SUBTREE}
    DEPENDS reading_capacity USES_TERMINAL)
add_custom_target(check_reading_capacity COMMAND reading_capacity --check WORKING_DIRECTORY ${WHITAKER_SUBTREE}
    DEPENDS reading_capacity USES_TERMINAL)
add_dependencies(gen check_reading_capacity)
