// INFO: whitaker.wasm, loaded once, and the inspector's calls into it (inspector/glue.cpp): load(words) looks up
//  the text, select(from, to) picks a run of it, view(v, first, count) draws it as HTML, search(query) marks the words
//  that match, and sampleStart, sample and time time the library alone.
import loadWhitaker from './whitaker.mjs';

async function fetchWithProgress(url, onProgress) {
  const response = await fetch(url);
  if (!response.ok) throw new Error(`${url}: ${response.status}`);
  const reader = response.body.getReader();
  const parts = [];
  let received = 0;
  for (;;) {
    const { done, value } = await reader.read();
    if (done) break;
    parts.push(value);
    received += value.length;
    onProgress(received);
  }
  const bytes = new Uint8Array(received);
  let offset = 0;
  for (const part of parts) {
    bytes.set(part, offset);
    offset += part.length;
  }
  return bytes;
}

export async function loadLibrary(onProgress) {
  const wasmBinary = await fetchWithProgress('whitaker.wasm', onProgress);
  const whitaker = await loadWhitaker({ wasmBinary });
  const load = whitaker.cwrap('inspector_load', 'string', ['number']);
  // NOTE: A text can be a whole book: it goes on the heap, since cwrap puts strings on the 64 KB stack.
  const onHeap = (text, call) => {
    const size = whitaker.lengthBytesUTF8(text) + 1;
    const pointer = whitaker._malloc(size);
    if (!pointer) throw new Error('out of memory');
    try {
      whitaker.stringToUTF8(text, pointer, size);
      return call(pointer);
    } finally {
      whitaker._free(pointer);
    }
  };
  return {
    load: words => onHeap(words.join('\n'), load),
    select: whitaker.cwrap('inspector_select', 'number', ['number', 'number']),
    view: whitaker.cwrap('inspector_view', 'string', ['number', 'number', 'number']),
    search: whitaker.cwrap('inspector_search', 'string', ['string']),
    sampleStart: whitaker.cwrap('inspector_sample_start', null, ['number']),
    sample: whitaker.cwrap('inspector_sample', 'number', []),
    time: whitaker.cwrap('inspector_time', 'number', []),
    binding: whitaker.cwrap('inspector_binding', null, ['number']),
  };
}
