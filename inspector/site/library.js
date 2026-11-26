// INFO: The page's library (../page.cpp): read(words) reads a text once and gives its counts, word(i) the readings of
//  its word i, lookup(word) one word outside the text, api(word, binding) the api window's calls. Its wasm is fetched
//  here, so the clock can show it coming in.
import loadWhitaker from './whitaker.mjs';
import { loadWasm, withHeapString } from './wasm.js';

export async function loadLibrary(onProgress) {
  const whitaker = await loadWasm(loadWhitaker, 'whitaker.wasm', onProgress);
  const readText = whitaker.cwrap('read_text', 'string', ['number']);
  const word = whitaker.cwrap('word', 'string', ['number']);
  const lookup = whitaker.cwrap('lookup', 'string', ['string']);
  const api = whitaker.cwrap('api', 'string', ['string', 'number']);
  return {
    read: words => withHeapString(whitaker, words.join('\n'), pointer => JSON.parse(readText(pointer))),
    word: i => JSON.parse(word(i)),
    lookup: spelling => JSON.parse(lookup(spelling)),
    api: (spelling, binding) => JSON.parse(api(spelling, binding)),
  };
}
