# INFO: whitaker.mjs and whitaker.wasm for the browser, built with Emscripten, into build-wasm/site beside the
#  inspector's page (site/), which is copied there:
#   source ~/emsdk/emsdk_env.sh
#   emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-wasm
# NOTE: Release, not MinSizeRel: about 25% faster per word (1.6 against 2.1 µs over Gallic War 1) for 31 KB more.
add_executable(whitaker_wasm glue.cpp)
target_link_libraries(whitaker_wasm PRIVATE whitaker whitaker_warnings whitaker_internals)
target_compile_features(whitaker_wasm PRIVATE cxx_std_26)
# NOTE: The library and the glue must agree: the trace changes the readings' layout.
target_compile_definitions(whitaker PRIVATE WHITAKER_TRACE)
target_compile_definitions(whitaker_wasm PRIVATE WHITAKER_TRACE)
# NOTE: One directory with the page and the library, so `python3 -m http.server -d build-wasm/site` serves it all.
set(WHITAKER_SITE ${CMAKE_BINARY_DIR}/site)
set_target_properties(whitaker_wasm PROPERTIES OUTPUT_NAME whitaker SUFFIX .mjs RUNTIME_OUTPUT_DIRECTORY ${WHITAKER_SITE})
foreach(file index.html style.css app.js library.js text.js views.js highlights.js ask.js sentence.js icon.svg)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/site/${file} ${WHITAKER_SITE}/${file} COPYONLY)
endforeach()
target_link_options(whitaker_wasm PRIVATE
    -sMODULARIZE
    -sEXPORT_ES6
    -sEXPORT_NAME=loadWhitaker
    -sENVIRONMENT=web,worker,node
    -sALLOW_MEMORY_GROWTH
    -sFILESYSTEM=0
    -sEXPORTED_FUNCTIONS=_inspector_load,_inspector_select,_inspector_view,_inspector_search,_inspector_time,_inspector_binding,_inspector_marks,_malloc,_free
    -sEXPORTED_RUNTIME_METHODS=cwrap,lengthBytesUTF8,stringToUTF8
)
