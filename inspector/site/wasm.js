export async function loadWasm(load, filename, onProgress) {
  const response = await fetch(filename);
  if (!response.ok) throw new Error(`${filename}: ${response.status}`);
  const reader = response.body.getReader();
  const parts = [];
  for (let received = 0; ;) {
    const { done, value } = await reader.read();
    if (done) break;
    parts.push(value);
    received += value.length;
    onProgress(received);
  }
  const bytes = await new Blob(parts).arrayBuffer();
  // NOTE: The inspector's loader defaults to the reader's whitaker.wasm unless given its own instance.
  return load({
    instantiateWasm: (imports, done) => {
      WebAssembly.instantiate(bytes, imports).then(r => done(r.instance));
      return {};
    },
  });
}

// NOTE: A whole text can exceed the 64 KB stack. The inspector also exports no stack helpers for cwrap.
export function withHeapString(wasm, text, use) {
  const size = wasm.lengthBytesUTF8(text) + 1;
  const pointer = wasm._malloc(size);
  if (!pointer) throw new Error('out of memory');
  try {
    wasm.stringToUTF8(text, pointer, size);
    return use(pointer);
  } finally {
    wasm._free(pointer);
  }
}
