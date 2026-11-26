# INFO: The site for the browser, built with Emscripten into build-wasm/site: the page (site/, copied there), the
#  reader's library (whitaker.mjs, whitaker.wasm) and the traced build (inspector.mjs, inspector.wasm) that Trace,
#  Stats and Sentence fetch the first time they are opened:
#   source ~/emsdk/emsdk_env.sh
#   emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-wasm
# NOTE: Release, not MinSizeRel: about 25% faster per word (1.6 against 2.1 µs over Gallic War 1) for 31 KB more.
# NOTE: One directory with the page and both builds, so `python3 -m http.server -d build-wasm/site` serves it all.
set(WHITAKER_SITE ${CMAKE_BINARY_DIR}/site)

add_executable(whitaker_wasm page.cpp)
target_link_libraries(whitaker_wasm PRIVATE whitaker)
target_compile_features(whitaker_wasm PRIVATE cxx_std_23)
set_target_properties(whitaker_wasm PROPERTIES OUTPUT_NAME whitaker)

add_executable(whitaker_inspector_wasm glue.cpp)
target_link_libraries(whitaker_inspector_wasm PRIVATE whitaker_traced whitaker_warnings whitaker_internals)
target_compile_features(whitaker_inspector_wasm PRIVATE cxx_std_26)
target_compile_definitions(whitaker_inspector_wasm PRIVATE WHITAKER_TRACE)
set_target_properties(whitaker_inspector_wasm PROPERTIES OUTPUT_NAME inspector)

foreach(target whitaker_wasm whitaker_inspector_wasm)
    set_target_properties(${target} PROPERTIES SUFFIX .mjs RUNTIME_OUTPUT_DIRECTORY ${WHITAKER_SITE})
    target_link_options(${target} PRIVATE
        --no-entry
        -sMODULARIZE
        -sEXPORT_ES6
        -sEXPORT_NAME=loadWhitaker
        -sENVIRONMENT=web,node
        -sALLOW_MEMORY_GROWTH
        -sFILESYSTEM=0
        -sEXPORTED_RUNTIME_METHODS=cwrap,lengthBytesUTF8,stringToUTF8
    )
endforeach()
target_link_options(whitaker_wasm PRIVATE -sEXPORTED_FUNCTIONS=_read_text,_word,_lookup,_api,_malloc,_free)
set(WHITAKER_INSPECTOR_EXPORTS _inspector_load _inspector_select _inspector_view _inspector_time _inspector_marks
    _malloc _free)
list(JOIN WHITAKER_INSPECTOR_EXPORTS "," WHITAKER_INSPECTOR_EXPORTS)
target_link_options(whitaker_inspector_wasm PRIVATE -sEXPORTED_FUNCTIONS=${WHITAKER_INSPECTOR_EXPORTS})

foreach(file index.html style.css icon.svg books.json api.js debug.js dom.js
        finder.js head.js library.js numbers.js page.js panes.js popup.js reading-library.js
        tei.js text.js wasm.js windows.js sentence/ask.js sentence/highlights.js
        sentence/sentence.js sentence/text.js)
    configure_file(site/${file} ${WHITAKER_SITE}/${file} COPYONLY)
endforeach()
