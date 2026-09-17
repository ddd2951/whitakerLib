# The version has one source, include/whitaker.h; everything derives from
# what this reads. codegen/ calls it too, being its own project.
#
#   whitaker_read_version(<out-variable> <path-to-whitaker.h>)

function(whitaker_read_version out_variable header)
    if(NOT EXISTS "${header}")
        message(FATAL_ERROR "whitaker_read_version: no such header: ${header}")
    endif()

    # Reading a file at configure time creates no dependency on it.
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${header}")

    # Two components; the project has no patch number yet.
    foreach(component MAJOR MINOR)
        file(STRINGS "${header}" matched
             REGEX "^#define[ \t]+WHITAKER_VERSION_${component}[ \t]+[0-9]+[ \t]*$")
        list(LENGTH matched count)
        if(NOT count EQUAL 1)
            message(FATAL_ERROR
                "whitaker_read_version: ${header} must define "
                "WHITAKER_VERSION_${component} exactly once; found ${count}")
        endif()
        string(REGEX REPLACE "^.*[ \t]([0-9]+)[ \t]*$" "\\1"
               ${component} "${matched}")
    endforeach()

    set(${out_variable} "${MAJOR}.${MINOR}" PARENT_SCOPE)
endfunction()
