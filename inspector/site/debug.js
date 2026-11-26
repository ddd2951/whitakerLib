// INFO: The inspector's traced build (inspector.mjs), fetched the first time a window needs it, and its views
//  (inspector/glue.cpp). A text is read into it once and kept until a word or another text replaces it.
import loadInspector from './inspector.mjs';
import { loadWasm, withHeapString } from './wasm.js';

const READINGS = 0;
const TRACE = 2;
const STATISTICS = 3;

let inspector = null;

export async function load(onProgress) {
  if (inspector) return inspector;
  const w = await loadWasm(loadInspector, 'inspector.wasm', onProgress);
  const loadOnHeap = w.cwrap('inspector_load', 'string', ['number']);
  const loadWords = text => withHeapString(w, text, loadOnHeap);
  const select = w.cwrap('inspector_select', 'number', ['number', 'number']);
  const view = w.cwrap('inspector_view', 'string', ['number', 'number', 'number']);
  const time = w.cwrap('inspector_time', 'number', []);
  const marks = w.cwrap('inspector_marks', 'string', []);
  // NOTE: The views time what is loaded once (inspector_time), to fill in where the time goes. A text stays loaded
  //  until a word or another text replaces it, so the trace and the statistics share one read.
  let loaded = null;
  let groups = 0;
  inspector = {
    word(word) {
      loadWords(word);
      loaded = null;
      select(0, 0);
      time();
      return view(TRACE, 0, 1);
    },
    // INFO: The whole text: how many groups its trace view has; then the view of groups first to first + count.
    text(words) {
      if (loaded !== words) {
        loadWords(words.join('\n'));
        loaded = words;
      }
      groups = select(0, words.length - 1);
      time();
      return groups;
    },
    slice: (first, count) => view(TRACE, first, count),
    // INFO: For the sentence view (sentence/): each word's marks, four hex digits a word, and word i's readings as
    //  HTML.
    marks: () => marks(),
    readings(i) {
      select(i, i);
      return view(READINGS, 0, 1);
    },
    statistics: () => view(STATISTICS, 0, 0),
  };
  return inspector;
}
