cmake_minimum_required(VERSION 3.21)

file(STRINGS include/whitaker.h WHITAKER_VERSION
    REGEX "^#define WHITAKER_VERSION_(MAJOR|MINOR|PATCH) ")
string(REGEX REPLACE "[^;]* ([0-9]+)" "\\1" WHITAKER_VERSION "${WHITAKER_VERSION}")
list(JOIN WHITAKER_VERSION . WHITAKER_VERSION)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS include/whitaker.h)

project(whitaker VERSION ${WHITAKER_VERSION} LANGUAGES CXX C)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    message(FATAL_ERROR
        "whitaker builds with GCC or Clang, not ${CMAKE_CXX_COMPILER_ID}")
endif()

include(GNUInstallDirs)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WHITAKER_BUILD_TESTING "Build the test suite" ${PROJECT_IS_TOP_LEVEL})
option(WHITAKER_INSTALL "Generate the install and package rules"
    ${PROJECT_IS_TOP_LEVEL})
option(WHITAKER_WERROR "Treat warnings as errors" ${PROJECT_IS_TOP_LEVEL})
option(WHITAKER_SANITIZE "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(WHITAKER_BUILD_TESTING)
    enable_testing()
endif()

include(warnings.cmake)
add_library(whitaker_warnings INTERFACE)

target_compile_options(whitaker_warnings INTERFACE
    $<$<BOOL:${WHITAKER_WERROR}>:-Werror>
    "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:${WHITAKER_WARNINGS};${WHITAKER_WARNINGS_C}>"
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:${WHITAKER_WARNINGS};${WHITAKER_WARNINGS_CXX}>"
    "$<$<COMPILE_LANG_AND_ID:C,GNU>:${WHITAKER_WARNINGS_GNU}>"
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:${WHITAKER_WARNINGS_GNU};${WHITAKER_WARNINGS_GNU_CXX}>"
    "$<$<COMPILE_LANG_AND_ID:C,Clang>:${WHITAKER_WARNINGS_CLANG}>"
    "$<$<COMPILE_LANG_AND_ID:CXX,Clang>:${WHITAKER_WARNINGS_CLANG}>"
)

add_library(whitaker_internals INTERFACE)
target_compile_features(whitaker_internals INTERFACE cxx_std_23)
target_include_directories(whitaker_internals INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/latin)
target_compile_options(whitaker_internals INTERFACE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti>)

function(whitaker_sanitize target)
    if(WHITAKER_SANITIZE)
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined -fno-sanitize-recover=undefined)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

add_library(whitaker SHARED
    src/search/addon_fallback.cpp
    src/search/embedded_image.cpp
    src/search/relationship_image.cpp
    src/search/syncope_lookup.cpp
    src/roman.cpp
    src/whitaker.cpp
)
add_library(Whitaker::whitaker ALIAS whitaker)

set_target_properties(whitaker PROPERTIES
    EXPORT_NAME whitaker
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION}
)

set(WHITAKER_DATA_IMAGE
    "${CMAKE_CURRENT_SOURCE_DIR}/data/whitaker.dat")
if(NOT EXISTS "${WHITAKER_DATA_IMAGE}")
    message(FATAL_ERROR "Missing generated lookup data: ${WHITAKER_DATA_IMAGE}")
endif()
# NOTE: #embed takes offsets and limits from the preprocessor alone, so the
#  string pool's place is read here from the image's directory: Strings is
#  section 13 (relationship_schema.hpp), each directory entry 24 bytes after
#  the 48-byte header. embedded_image.cpp checks both against the image.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${WHITAKER_DATA_IMAGE}")
file(READ "${WHITAKER_DATA_IMAGE}" entry_hex OFFSET 360 LIMIT 16 HEX)
foreach(field AT BYTES)
    # A little-endian u64, most significant byte first for math().
    set(hex "")
    foreach(byte RANGE 7 0 -1)
        math(EXPR hex_index "2 * ${byte}")
        string(SUBSTRING "${entry_hex}" ${hex_index} 2 pair)
        string(APPEND hex "${pair}")
    endforeach()
    math(EXPR WHITAKER_IMAGE_STRINGS_${field} "0x${hex}")
    string(SUBSTRING "${entry_hex}" 16 -1 entry_hex)
endforeach()
file(CONFIGURE OUTPUT image_layout.h CONTENT
    "// Generated from data/whitaker.dat's directory by CMakeLists.txt.
#pragma once
#define WHITAKER_IMAGE_FILE \"${WHITAKER_DATA_IMAGE}\"
#define WHITAKER_IMAGE_STRINGS_AT ${WHITAKER_IMAGE_STRINGS_AT}
#define WHITAKER_IMAGE_STRINGS_BYTES ${WHITAKER_IMAGE_STRINGS_BYTES}
")
set_source_files_properties(src/search/embedded_image.cpp
    PROPERTIES OBJECT_DEPENDS "${WHITAKER_DATA_IMAGE}")
# NOTE: The compiler decodes every grammar there, 4M to 8M of Clang's steps
#  against a default of 1M; GCC's default of 33M is enough.
set_property(SOURCE src/search/embedded_image.cpp APPEND PROPERTY
    COMPILE_OPTIONS $<$<CXX_COMPILER_ID:Clang>:-fconstexpr-steps=33554432>)
target_include_directories(whitaker PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(whitaker PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(NOT WIN32)
    target_compile_options(whitaker PRIVATE
        -fvisibility=hidden
        $<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>
    )
endif()
target_link_libraries(whitaker PRIVATE
    $<BUILD_INTERFACE:whitaker_warnings> $<BUILD_INTERFACE:whitaker_internals>)
target_compile_definitions(whitaker PRIVATE WHITAKER_BUILDING)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_options(whitaker PRIVATE
        -fno-semantic-interposition
    )
    target_link_options(whitaker PRIVATE
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/exports.map"
        "LINKER:-z,defs"
        "LINKER:-z,relro"
        "LINKER:-z,now"
    )
    set_property(TARGET whitaker APPEND PROPERTY LINK_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/exports.map"
    )
endif()
whitaker_sanitize(whitaker)

if(WHITAKER_BUILD_TESTING)
    function(whitaker_test target)
        target_link_libraries(${target} PRIVATE whitaker_warnings)
        target_compile_options(${target} PRIVATE -UNDEBUG)
        whitaker_sanitize(${target})
        add_test(NAME ${target} COMMAND ${target} ${ARGN})
    endfunction()

    add_executable(whitaker_test test/test.c)
    target_link_libraries(whitaker_test PRIVATE whitaker)
    whitaker_test(whitaker_test)

    if(WHITAKER_INSTALL)
        set(WHITAKER_EXAMPLE_FLAGS "-Wall -Wextra -Wpedantic -Werror")
        if(WHITAKER_SANITIZE)
            string(APPEND WHITAKER_EXAMPLE_FLAGS " -fsanitize=address,undefined")
        endif()
        add_test(NAME whitaker_examples
            COMMAND ${CMAKE_COMMAND}
                -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}
                -DBUILD_DIR=${CMAKE_CURRENT_BINARY_DIR}
                -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/examples
                -DC_COMPILER=${CMAKE_C_COMPILER}
                -DCXX_COMPILER=${CMAKE_CXX_COMPILER}
                -DFLAGS=${WHITAKER_EXAMPLE_FLAGS}
                -DCONFIG=$<CONFIG>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/test/examples.cmake)
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        add_test(NAME whitaker_build_boundary
            COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/test/build_boundary.sh
                $<TARGET_FILE:whitaker>)
    endif()
endif()

if(WHITAKER_INSTALL)

# The steps from ${pcfiledir} back to the prefix depend on the libdir
# (lib, lib64, lib/x86_64-linux-gnu), so compute them.
file(RELATIVE_PATH WHITAKER_PC_PREFIX
    "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig" "${CMAKE_INSTALL_PREFIX}")
string(REGEX REPLACE "/$" "" WHITAKER_PC_PREFIX "${WHITAKER_PC_PREFIX}")
# An absolute install dir stays as given; a relative one is under the prefix.
foreach(dir LIBDIR INCLUDEDIR)
    if(IS_ABSOLUTE "${CMAKE_INSTALL_${dir}}")
        set(WHITAKER_PC_${dir} "${CMAKE_INSTALL_${dir}}")
    else()
        set(WHITAKER_PC_${dir} "\${prefix}/${CMAKE_INSTALL_${dir}}")
    endif()
endforeach()
configure_file(whitaker.pc.in whitaker.pc @ONLY)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/WhitakerConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(TARGETS whitaker EXPORT WhitakerTargets)
install(FILES
    include/whitaker.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/whitaker.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
# The EUPL requires its text with every copy; Whitaker's notice covers the data.
install(FILES LICENSE data/source/LICENCE.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/whitaker
)
# The library has no dependencies, so its targets file is the whole package.
install(EXPORT WhitakerTargets
    FILE WhitakerConfig.cmake
    NAMESPACE Whitaker::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Whitaker
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/WhitakerConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Whitaker
)

endif()
