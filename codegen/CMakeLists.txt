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
    ${WHITAKER_SUBTREE} ${WHITAKER_SUBTREE}/src ${WHITAKER_SUBTREE}/latin)

add_executable(gen
    src/main.cpp
    src/error/error.cpp
    src/util/file_load.cpp
    src/types/latin_test.cpp
    src/publish/atomic_publication.cpp
    src/source/source.cpp
    src/word/word.cpp
    src/expand/expand.cpp
    src/expand/common/reading.cpp
    src/expand/facts/adverb_corrections.cpp
    src/expand/facts/candidates.cpp
    src/expand/facts/forms.cpp
    src/expand/facts/readings.cpp
    src/expand/facts/sorted.cpp
    src/expand/facts/swept.cpp
    src/expand/facts/uniques.cpp
    src/emitter/emitter.cpp
    src/emitter/image_file.cpp
    src/emitter/relationship_emitter/image_builder.cpp
    src/emitter/relationship_emitter/image_writer.cpp
    src/emitter/relationship_emitter/relationship_index.cpp
    src/emitter/relationship_emitter/relationship_post_validation.cpp
    src/emitter/relationship_emitter/string_section.cpp)
target_include_directories(gen PRIVATE src)
target_compile_features(gen PRIVATE cxx_std_26)
target_compile_options(gen PRIVATE -freflection ${GEN_OPTIONS})
target_link_libraries(gen PRIVATE gen_runtime)

add_custom_target(image COMMAND gen WORKING_DIRECTORY ${WHITAKER_SUBTREE}
    USES_TERMINAL)
