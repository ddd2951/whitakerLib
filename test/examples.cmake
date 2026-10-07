# WHITAKER_UPDATE_EXPECTED=1 in the environment writes the examples' output
# over examples/expected/ instead of comparing it.

function(run)
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE status
    OUTPUT_VARIABLE output ERROR_VARIABLE output)
  if(NOT status EQUAL 0)
    message(FATAL_ERROR "${ARGN}\n${output}")
  endif()
endfunction()

function(expect name)
  set(expected "${SOURCE_DIR}/examples/expected/${name}.txt")
  execute_process(COMMAND ${ARGN} RESULT_VARIABLE status
    OUTPUT_VARIABLE actual ERROR_VARIABLE errors)
  if(NOT status EQUAL 0)
    message(FATAL_ERROR "${name} example failed (${status}):\n${errors}")
  endif()
  if(DEFINED ENV{WHITAKER_UPDATE_EXPECTED})
    file(WRITE "${expected}" "${actual}")
    return()
  endif()
  file(READ "${expected}" wanted)
  if(NOT actual STREQUAL wanted)
    file(WRITE "${WORK_DIR}/${name}.txt" "${actual}")
    execute_process(COMMAND diff -u "${expected}" "${WORK_DIR}/${name}.txt")
    message(FATAL_ERROR "${name} example output changed (diff above)")
  endif()
endfunction()

if(CONFIG)
  set(config --config "${CONFIG}")
endif()
file(REMOVE_RECURSE "${WORK_DIR}")
run(${CMAKE_COMMAND} --install "${BUILD_DIR}" ${config}
  --prefix "${WORK_DIR}/prefix")
run(${CMAKE_COMMAND} -S "${SOURCE_DIR}/examples" -B "${WORK_DIR}/build"
  "-DCMAKE_PREFIX_PATH=${WORK_DIR}/prefix"
  "-DCMAKE_C_COMPILER=${C_COMPILER}" "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}"
  "-DCMAKE_C_FLAGS=${FLAGS}" "-DCMAKE_CXX_FLAGS=${FLAGS}"
  "-DCMAKE_EXE_LINKER_FLAGS=${FLAGS}"
  "-DCMAKE_BUILD_TYPE=${CONFIG}")
run(${CMAKE_COMMAND} --build "${WORK_DIR}/build")

expect(c "${WORK_DIR}/build/whitaker-c-example"
  INPUT_FILE "${SOURCE_DIR}/examples/expected/c.words")
expect(cpp "${WORK_DIR}/build/whitaker-cpp-example")
