cmake_minimum_required(VERSION 3.20)

# The version is read from include/whitaker.h; whitaker.pc, the package
# version file, and the SONAME all derive from it.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/WhitakerVersion.cmake)
whitaker_read_version(WHITAKER_VERSION
    ${CMAKE_CURRENT_SOURCE_DIR}/include/whitaker.h)

project(whitaker VERSION ${WHITAKER_VERSION} LANGUAGES CXX C ASM)

# The image is embedded with GNU assembly and hidden by a linker version script.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR
   NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
    message(FATAL_ERROR
        "whitaker builds on GNU/Linux with GCC or Clang; "
        "${CMAKE_SYSTEM_NAME} with ${CMAKE_CXX_COMPILER_ID} is not supported")
endif()

include(GNUInstallDirs)

# A consumer that adds this directory gets the library only: no tests,
# install rules, or fatal warnings.
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(WHITAKER_TOP_LEVEL_DEFAULT ON)
else()
    set(WHITAKER_TOP_LEVEL_DEFAULT OFF)
endif()
option(WHITAKER_BUILD_TESTING "Build the test suite"
    ${WHITAKER_TOP_LEVEL_DEFAULT})
option(WHITAKER_INSTALL "Generate the install and package rules"
    ${WHITAKER_TOP_LEVEL_DEFAULT})
option(WHITAKER_WERROR "Treat warnings as errors"
    ${WHITAKER_TOP_LEVEL_DEFAULT})
option(WHITAKER_BUILD_TOOLS "Build command-line tools" OFF)
option(WHITAKER_SANITIZE "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(WHITAKER_BUILD_TESTING)
    enable_testing()
endif()

# --- Warning policy ---

# -Wswitch-enum and -Wpadded are left out on purpose: -Wswitch covers the
# dangerous case, and the layout that matters is the serialized image.
add_library(whitaker_warnings INTERFACE)

set(WHITAKER_WARNINGS
    -Wall -Wextra -Wpedantic
    -Wshadow -Wcast-align -Wcast-qual -Wdouble-promotion -Wfloat-equal
    -Wformat=2 -Wnull-dereference -Wredundant-decls -Wundef
    -Wunused-macros -Wmissing-declarations
    -Wconversion -Wsign-conversion)
# GCC rejects the C++-only flags in C.
set(WHITAKER_WARNINGS_CXX
    -Wold-style-cast -Wnon-virtual-dtor -Woverloaded-virtual -Wsuggest-override
    -Wextra-semi -Wzero-as-null-pointer-constant)
set(WHITAKER_WARNINGS_C -Wstrict-prototypes -Wmissing-prototypes)
# GCC spells these with a level; Clang has only the plain form.
# -Warith-conversion is GCC's alone.
set(WHITAKER_WARNINGS_GNU
    -Wduplicated-cond -Wduplicated-branches -Wlogical-op
    -Wimplicit-fallthrough=5 -Wunused-const-variable=2 -Warith-conversion)
set(WHITAKER_WARNINGS_CLANG -Wimplicit-fallthrough -Wunused-const-variable)
set(WHITAKER_WARNINGS_GNU_CXX -Wuseless-cast)

# -Werror is ours, not a packager's: a newer compiler will find something.
target_compile_options(whitaker_warnings INTERFACE
    $<$<BOOL:${WHITAKER_WERROR}>:-Werror>
    "$<$<COMPILE_LANG_AND_ID:C,GNU,Clang>:${WHITAKER_WARNINGS};${WHITAKER_WARNINGS_C}>"
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU,Clang>:${WHITAKER_WARNINGS};${WHITAKER_WARNINGS_CXX}>"
    "$<$<COMPILE_LANG_AND_ID:C,GNU>:${WHITAKER_WARNINGS_GNU}>"
    "$<$<COMPILE_LANG_AND_ID:CXX,GNU>:${WHITAKER_WARNINGS_GNU};${WHITAKER_WARNINGS_GNU_CXX}>"
    "$<$<COMPILE_LANG_AND_ID:C,Clang>:${WHITAKER_WARNINGS_CLANG}>"
    "$<$<COMPILE_LANG_AND_ID:CXX,Clang>:${WHITAKER_WARNINGS_CLANG}>"
)

# --- Library ---

add_library(whitaker SHARED
    src/search/addon_fallback.cpp
    src/search/relationship_image.cpp
    src/roman.cpp
    src/whitaker.cpp
)
add_library(Whitaker::whitaker ALIAS whitaker)

target_compile_features(whitaker PRIVATE cxx_std_23)
set_target_properties(whitaker PROPERTIES
    CXX_EXTENSIONS OFF
    EXPORT_NAME whitaker
    SOVERSION ${PROJECT_VERSION_MAJOR}
    VERSION ${PROJECT_VERSION}
)

set(WHITAKER_DATA_IMAGE
    "${CMAKE_CURRENT_SOURCE_DIR}/data/whitaker.dat")
if(NOT EXISTS "${WHITAKER_DATA_IMAGE}")
    message(FATAL_ERROR "Missing generated lookup data: ${WHITAKER_DATA_IMAGE}")
endif()
# The library embeds the checked-in image; codegen/ is configured separately.
get_source_file_property(WHITAKER_DATA_IMAGE_GENERATED
    "${WHITAKER_DATA_IMAGE}" GENERATED)
if(WHITAKER_DATA_IMAGE_GENERATED)
    message(FATAL_ERROR
        "A target in this build produces ${WHITAKER_DATA_IMAGE}. "
        "The library must embed the checked-in image.")
endif()
configure_file(src/search/layout_image.S.in layout_image.S @ONLY)
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/layout_image.S"
    PROPERTIES OBJECT_DEPENDS "${WHITAKER_DATA_IMAGE}")
target_sources(whitaker PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/layout_image.S")

target_include_directories(whitaker
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(whitaker PRIVATE
    -fno-exceptions
    -fno-rtti
    -fvisibility=hidden
    -fvisibility-inlines-hidden
    # Every symbol outside exports.map is hidden, so calls bind directly.
    -fno-semantic-interposition
)
# The warning policy is build-time only; the exported target must not carry it.
target_link_libraries(whitaker PRIVATE $<BUILD_INTERFACE:whitaker_warnings>)
target_compile_definitions(whitaker PRIVATE WHITAKER_EMBEDDED_ONLY)
target_link_options(whitaker PRIVATE
    "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/exports.map"
    # No undefined symbol; every relocation resolved before the library runs.
    "LINKER:-z,defs"
    "LINKER:-z,relro"
    "LINKER:-z,now"
)
set_property(TARGET whitaker APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/exports.map"
)

if(WHITAKER_SANITIZE)
    target_compile_options(whitaker PRIVATE -fsanitize=address,undefined)
    target_link_options(whitaker PRIVATE -fsanitize=address,undefined)
endif()

# --- Optional command-line tool ---

if(WHITAKER_BUILD_TOOLS)
    add_executable(whitaker-lookup tools/lookup/main.c)
    target_link_libraries(whitaker-lookup PRIVATE
        Whitaker::whitaker whitaker_warnings)
    if(WHITAKER_SANITIZE)
        target_compile_options(whitaker-lookup PRIVATE -fsanitize=address,undefined)
        target_link_options(whitaker-lookup PRIVATE -fsanitize=address,undefined)
    endif()
endif()

# --- Tests ---

if(WHITAKER_BUILD_TESTING)
    add_executable(whitaker_test test/test.c)
    target_link_libraries(whitaker_test PRIVATE whitaker whitaker_warnings)
    target_compile_options(whitaker_test PRIVATE -UNDEBUG)
    if(WHITAKER_SANITIZE)
        target_compile_options(whitaker_test PRIVATE -fsanitize=address,undefined)
        target_link_options(whitaker_test PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME whitaker_test COMMAND whitaker_test)

    # Compiled against test/malformed_image.S instead of the image, so it never
    # links `whitaker`.
    add_executable(whitaker_malformed_image_test
        test/malformed_image.S
        test/malformed_image_test.c
        src/search/addon_fallback.cpp
        src/search/relationship_image.cpp
        src/roman.cpp
        src/whitaker.cpp
    )
    target_compile_features(whitaker_malformed_image_test PRIVATE cxx_std_23)
    set_target_properties(whitaker_malformed_image_test PROPERTIES
        CXX_EXTENSIONS OFF)
    target_include_directories(whitaker_malformed_image_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    target_compile_options(whitaker_malformed_image_test PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti> -UNDEBUG
    )
    target_link_libraries(whitaker_malformed_image_test PRIVATE
        whitaker_warnings)
    target_compile_definitions(whitaker_malformed_image_test PRIVATE
        WHITAKER_EMBEDDED_ONLY)
    if(WHITAKER_SANITIZE)
        target_compile_options(whitaker_malformed_image_test PRIVATE
            -fsanitize=address,undefined)
        target_link_options(whitaker_malformed_image_test PRIVATE
            -fsanitize=address,undefined)
    endif()
    add_test(NAME whitaker_malformed_image_test
             COMMAND whitaker_malformed_image_test)

    # The embedded span is exactly the checked-in image, and nothing in the
    # library can open a data file instead.
    add_test(NAME whitaker_build_boundary
        COMMAND ${CMAKE_COMMAND}
            -DLIBRARY=$<TARGET_FILE:whitaker>
            -DIMAGE=${WHITAKER_DATA_IMAGE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/test/build_boundary.cmake
    )
endif()

# --- Install and packaging ---

if(WHITAKER_INSTALL)

# The steps from ${pcfiledir} back to the prefix depend on the libdir
# (lib, lib64, lib/x86_64-linux-gnu), so compute them.
file(RELATIVE_PATH WHITAKER_PC_PREFIX
    "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig" "${CMAKE_INSTALL_PREFIX}")
string(REGEX REPLACE "/$" "" WHITAKER_PC_PREFIX "${WHITAKER_PC_PREFIX}")
configure_file(whitaker.pc.in whitaker.pc @ONLY)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/WhitakerConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/WhitakerConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Whitaker
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/WhitakerConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(TARGETS whitaker
    EXPORT WhitakerTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(WHITAKER_BUILD_TOOLS)
    install(TARGETS whitaker-lookup
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
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
install(EXPORT WhitakerTargets
    FILE WhitakerTargets.cmake
    NAMESPACE Whitaker::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Whitaker
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/WhitakerConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/WhitakerConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Whitaker
)

endif()
