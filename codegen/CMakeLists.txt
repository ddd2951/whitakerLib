cmake_minimum_required(VERSION 3.28)
project(gen LANGUAGES CXX)

set(WHITAKER_SUBTREE ${CMAKE_CURRENT_SOURCE_DIR}/..)

# The banner reports the library's version, read from the same header.
include(${WHITAKER_SUBTREE}/cmake/WhitakerVersion.cmake)
whitaker_read_version(WHITAKER_VERSION ${WHITAKER_SUBTREE}/include/whitaker.h)

# --- Compile policy ---

# Same contract as the library: no exceptions, no RTTI.
# -Wshadow is absent: every `template for` expansion trips it (GCC 16.2.1).
add_library(gen_policy INTERFACE)
target_compile_options(gen_policy INTERFACE -fno-exceptions -fno-rtti)

set(GEN_WARNINGS
    -Wall -Wextra -Wpedantic -Werror
    -Wcast-align -Wcast-qual -Wdouble-promotion -Wfloat-equal -Wformat=2
    -Wnull-dereference -Wredundant-decls -Wundef
    -Wunused-macros -Wmissing-declarations
    -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual -Wsuggest-override
    -Wextra-semi -Wzero-as-null-pointer-constant
    -Wconversion -Wsign-conversion)
# GCC spells these with a level; Clang has only the plain form.
# -Warith-conversion is GCC's alone.
set(GEN_WARNINGS_GNU
    -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wuseless-cast
    -Wimplicit-fallthrough=5 -Wunused-const-variable=2 -Warith-conversion)
set(GEN_WARNINGS_CLANG -Wimplicit-fallthrough -Wunused-const-variable)

target_compile_options(gen_policy INTERFACE
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:${GEN_WARNINGS}>"
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:${GEN_WARNINGS_GNU}>"
    "$<$<COMPILE_LANG_AND_ID:CXX,Clang>:${GEN_WARNINGS_CLANG}>"
)

# --- What every generator target compiles against ---

# Generator headers are spelled from src/: "types/types.hpp",
# "gen/roster_tag.hpp".
add_library(gen_headers INTERFACE)
target_include_directories(gen_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)
# The headers use C++26 reflection.
target_compile_features(gen_headers INTERFACE cxx_std_26)
target_compile_options(gen_headers INTERFACE -freflection)

# --- Deliberate reaches into private runtime source ---

# These two targets are all the generator borrows from the library. Neither
# is linked into the library.

# facts.hpp: the corpus rules both sides must agree on.
add_library(gen_runtime_facts INTERFACE)
target_include_directories(gen_runtime_facts INTERFACE ${WHITAKER_SUBTREE}/src)

# The reader the library ships; the emitter reads its image back with it.
# Call sites spell the reach "src/search/...".
add_library(gen_runtime_relationship STATIC
    ${WHITAKER_SUBTREE}/src/search/relationship_image.cpp)
target_include_directories(gen_runtime_relationship PUBLIC ${WHITAKER_SUBTREE})
target_link_libraries(gen_runtime_relationship
    PUBLIC gen_runtime_facts
    PRIVATE gen_policy)

# main_config.hpp: corpus and output paths, and the version the run reports.
add_library(gen_config INTERFACE)
target_compile_definitions(gen_config INTERFACE
    GEN_DATA_DIR="${WHITAKER_SUBTREE}/data"
    GEN_VERSION="${WHITAKER_VERSION}")

include(CTest)

add_subdirectory(src/error)
add_subdirectory(src/util)
add_subdirectory(src/types)
add_subdirectory(src/publish)
add_subdirectory(src/gen/construction)
add_subdirectory(src/gen/tokenize)
add_subdirectory(src/gen/expand)
add_subdirectory(src/gen/emitter)

add_executable(gen
    src/main.cpp
    src/gen/gen.cpp
)

target_link_libraries(gen PRIVATE
    gen_types
    gen_error
    gen_publish
    gen_construction_wl_stable
    gen_tokenize_wl_stable
    gen_expand_wl_stable
    gen_emitter_wl_stable
)

target_link_libraries(gen PRIVATE gen_policy gen_config)

# Separate executables so each check can be run alone.
if(BUILD_TESTING)
    add_executable(gen_roster_tag_test src/gen/roster_tag_test.cpp)
    target_link_libraries(gen_roster_tag_test PRIVATE gen_headers gen_policy)
    add_test(NAME gen_roster_tag_test COMMAND gen_roster_tag_test)

    add_executable(gen_roster_test src/gen/roster_test.cpp)
    target_link_libraries(gen_roster_test PRIVATE
        gen_headers
        gen_config
        gen_runtime_facts
        gen_policy)
    add_test(NAME gen_roster_test COMMAND gen_roster_test)

endif()
