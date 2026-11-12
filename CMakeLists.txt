cmake_minimum_required(VERSION 3.21)

file(STRINGS include/whitaker.hpp WHITAKER_VERSION_LINE REGEX "^inline constexpr char kVersion")
string(REGEX MATCH "[0-9]+[.][0-9]+[.][0-9]+" WHITAKER_VERSION "${WHITAKER_VERSION_LINE}")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS include/whitaker.hpp)
project(whitaker VERSION ${WHITAKER_VERSION} LANGUAGES C CXX)

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
        # NOTE: Linux's -z,defs needs Clang's shared runtime; executables must use that same runtime.
        if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            target_link_options(${target} PRIVATE -shared-libsan)
        endif()
    endif()
endfunction()

# NOTE: The browser build links the library into one module.
if(EMSCRIPTEN)
    set(WHITAKER_LIBRARY_TYPE STATIC)
else()
    set(WHITAKER_LIBRARY_TYPE SHARED)
endif()
add_library(whitaker ${WHITAKER_LIBRARY_TYPE}
    src/parse/adverb.cpp
    src/parse/find_readings.cpp
    src/parse/result.cpp
    src/parse/respellings.cpp
    src/parse/word_readings.cpp
    src/search/addon_readings.cpp
    src/search/embedded_image.cpp
    src/search/relationship_image.cpp
    src/search/syncope_lookup.cpp
    src/whitaker.cpp
    bindings/c/whitaker_c.cpp
)
add_library(Whitaker::whitaker ALIAS whitaker)

# INFO: The embedded image's section places are link-time constants (gen loads its image at run time instead).
target_compile_definitions(whitaker PRIVATE WHITAKER_EMBEDDED_FRAME)

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
#  section 7 (relationship_schema.hpp), each directory entry 24 bytes after
#  the 40-byte header. embedded_image.cpp checks both against the image.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${WHITAKER_DATA_IMAGE}")
file(READ "${WHITAKER_DATA_IMAGE}" entry_hex OFFSET 208 LIMIT 16 HEX)
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
# INFO: The whole directory, so the library can use each section's place as a constant. 25 entries of offset,
#  bytes and count; embedded_image.cpp checks them against the image.
file(READ "${WHITAKER_DATA_IMAGE}" directory_hex OFFSET 40 LIMIT 600 HEX)
set(WHITAKER_IMAGE_DIRECTORY "")
foreach(field RANGE 0 74)
    set(hex "")
    foreach(byte RANGE 7 0 -1)
        math(EXPR hex_index "16 * ${field} + 2 * ${byte}")
        string(SUBSTRING "${directory_hex}" ${hex_index} 2 pair)
        string(APPEND hex "${pair}")
    endforeach()
    math(EXPR value "0x${hex}")
    math(EXPR position "${field} % 3")
    if(position EQUAL 0)
        string(APPEND WHITAKER_IMAGE_DIRECTORY "{${value}u, ")
    elseif(position EQUAL 1)
        string(APPEND WHITAKER_IMAGE_DIRECTORY "${value}u, ")
    else()
        string(APPEND WHITAKER_IMAGE_DIRECTORY "${value}u}, ")
    endif()
endforeach()
file(CONFIGURE OUTPUT image_layout.h CONTENT
    "// Generated from data/whitaker.dat's directory by CMakeLists.txt.
#pragma once
#define WHITAKER_IMAGE_FILE \"${WHITAKER_DATA_IMAGE}\"
#define WHITAKER_IMAGE_STRINGS_AT ${WHITAKER_IMAGE_STRINGS_AT}
#define WHITAKER_IMAGE_STRINGS_BYTES ${WHITAKER_IMAGE_STRINGS_BYTES}
#define WHITAKER_IMAGE_DIRECTORY ${WHITAKER_IMAGE_DIRECTORY}
")
file(SHA256 "${WHITAKER_DATA_IMAGE}" WHITAKER_IMAGE_SHA256)
file(CONFIGURE OUTPUT image_checksum.h CONTENT
    "// Generated from data/whitaker.dat by CMakeLists.txt.
#pragma once
#define WHITAKER_IMAGE_SHA256 \"${WHITAKER_IMAGE_SHA256}\"
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
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/bindings/c>
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

if(EMSCRIPTEN)
    add_subdirectory(inspector)
endif()

option(WHITAKER_BUILD_PYTHON "Build the Python binding; needs pybind11" OFF)
if(WHITAKER_BUILD_PYTHON)
    add_subdirectory(bindings/python)
endif()

if(WHITAKER_BUILD_TESTING)
    function(whitaker_test target)
        target_link_libraries(${target} PRIVATE whitaker_warnings)
        target_compile_options(${target} PRIVATE -UNDEBUG)
        whitaker_sanitize(${target})
        add_test(NAME ${target} COMMAND ${target} ${ARGN})
    endfunction()

    add_executable(whitaker_test test/test.cpp)
    target_link_libraries(whitaker_test PRIVATE whitaker)
    target_compile_features(whitaker_test PRIVATE cxx_std_23)
    whitaker_test(whitaker_test)

    add_executable(whitaker_c_test test/test_c.c)
    target_link_libraries(whitaker_c_test PRIVATE whitaker)
    target_compile_features(whitaker_c_test PRIVATE c_std_11)
    whitaker_test(whitaker_c_test)

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
    include/whitaker.hpp
    bindings/c/whitaker.h
    include/latin_types.hpp
    include/latin_generated.hpp
    include/latin_describe.hpp
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
