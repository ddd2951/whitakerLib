# Check that the library embeds the checked-in image and has no data-file I/O.
cmake_minimum_required(VERSION 3.20)

if(NOT EXISTS "${LIBRARY}")
    message(FATAL_ERROR "no library to inspect: ${LIBRARY}")
endif()
if(NOT EXISTS "${IMAGE}")
    message(FATAL_ERROR "no image to compare against: ${IMAGE}")
endif()

find_program(NM NAMES nm llvm-nm)
if(NOT NM)
    message(FATAL_ERROR "nm is required to check the boundary")
endif()

file(SIZE "${IMAGE}" image_bytes)

execute_process(
    COMMAND "${NM}" --defined-only "${LIBRARY}"
    OUTPUT_VARIABLE defined
    RESULT_VARIABLE status
    ERROR_VARIABLE  nm_error
)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "nm failed: ${nm_error}")
endif()
# The payload is assembled into .rodata.whitaker_layout, but the link folds
# every .rodata.* into .rodata, so the section name does not survive. The two
# boundary symbols do, and they are what the reader actually reads.
foreach(edge start end)
    if(NOT defined MATCHES "([0-9a-fA-F]+)[ \t]+[a-zA-Z][ \t]+whitaker_layout_image_${edge}")
        message(FATAL_ERROR
                "the library defines no whitaker_layout_image_${edge}: the "
                "image is not embedded where the reader expects it")
    endif()
    set(${edge} "0x${CMAKE_MATCH_1}")
endforeach()
math(EXPR span_bytes "${end} - ${start}")
if(NOT span_bytes EQUAL image_bytes)
    message(FATAL_ERROR
            "embedded span is ${span_bytes} bytes but ${IMAGE} is "
            "${image_bytes} bytes")
endif()

execute_process(
    COMMAND "${NM}" --dynamic --undefined-only "${LIBRARY}"
    OUTPUT_VARIABLE undefined
    RESULT_VARIABLE status
    ERROR_VARIABLE  nm_error
)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "nm failed: ${nm_error}")
endif()

set(openers
    open open64 openat openat64 creat creat64
    fopen fopen64 freopen freopen64 fdopen
    mmap mmap64 dlopen)
string(REPLACE "\n" ";" lines "${undefined}")
foreach(line IN LISTS lines)
    if(NOT line MATCHES "U[ \t]+([^ \t@]+)")
        continue()
    endif()
    set(symbol "${CMAKE_MATCH_1}")
    if(symbol IN_LIST openers OR symbol MATCHES "ifstream|filebuf|basic_fstream")
        message(FATAL_ERROR
                "the library resolves ${symbol}, so it can open a data file; "
                "an ordinary build must answer from the embedded image alone")
    endif()
endforeach()

message(STATUS "embedded span: ${span_bytes} bytes, matching ${IMAGE}")
message(STATUS "no file-opening symbol is resolved by the library")
