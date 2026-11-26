// INFO: Saved bodies and their small finder entries live in IndexedDB. Positions are written synchronously so a
//  refresh or pagehide can save the last scroll. Storage failures leave the current tab usable.
const prefix = 'whitakerlib.reader.';
const temporary = new Map();
const positions = new Map();
let database;
export let storageError = false;

function connect() {
  return database ??= new Promise((resolve, reject) => {
    const request = indexedDB.open('whitakerlib-reader', 1);
    request.onupgradeneeded = () => {
      request.result.createObjectStore('entries', { keyPath: 'id' });
      request.result.createObjectStore('texts');
    };
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error);
    request.onblocked = () => reject(new Error('Reading library is blocked'));
  });
}

async function transact(stores, mode, run) {
  try {
    const db = await connect();
    return await new Promise((resolve, reject) => {
      const tx = db.transaction(stores, mode);
      tx.onabort = () => reject(tx.error);
      let request;
      try { request = run(tx); }
      catch (error) { tx.abort(); reject(error); }
      tx.oncomplete = () => resolve(request?.result);
    });
  } catch {
    storageError = true;
    return undefined;
  }
}

export async function save(entry) {
  temporary.set(entry.id, entry);
  const { source, ...metadata } = entry;
  const saved = await transact(['entries', 'texts'], 'readwrite', tx => {
    tx.objectStore('texts').put(source, entry.id);
    return tx.objectStore('entries').put(metadata);
  });
  if (saved !== undefined && temporary.get(entry.id) === entry) temporary.delete(entry.id);
}

export async function load(id) {
  if (temporary.has(id)) return temporary.get(id);
  const pair = await transact(['entries', 'texts'], 'readonly', tx => {
    const meta = tx.objectStore('entries').get(id);
    const body = tx.objectStore('texts').get(id);
    return { get result() { return [meta.result, body.result]; } };
  });
  return pair?.[0] && typeof pair[1] === 'string' ? { ...pair[0], source: pair[1] } : null;
}

export async function entries() {
  const saved = await transact('entries', 'readonly', tx => tx.objectStore('entries').getAll());
  const byId = new Map((saved ?? []).map(entry => [entry.id, entry]));
  for (const { source, ...entry } of temporary.values()) byId.set(entry.id, entry);
  return [...byId.values()].map(entry => ({ ...entry, used: position(entry.id)?.used ?? 0 }))
    .sort((a, b) => b.used - a.used);
}

function read(key) {
  try { return JSON.parse(localStorage.getItem(prefix + key)); }
  catch { storageError = true; return positions.get(key) ?? null; }
}

function write(key, value) {
  try {
    if (value === null) localStorage.removeItem(prefix + key);
    else localStorage.setItem(prefix + key, JSON.stringify(value));
    positions.delete(key);
  } catch {
    storageError = true;
    positions.set(key, value);
  }
}

export const position = id => positions.has(id) ? positions.get(id) : read(id);
export const lastId = () => position('last');
export const savePosition = (id, place) => write(id, { ...position(id), ...place });

export function activate(id) {
  savePosition(id, { used: Date.now() });
  write('last', id);
}

export async function remove(id) {
  temporary.delete(id);
  write(id, null);
  if (lastId() === id) write('last', null);
  await transact(['entries', 'texts'], 'readwrite', tx => {
    tx.objectStore('texts').delete(id);
    tx.objectStore('entries').delete(id);
  });
}

export async function clear() {
  temporary.clear();
  positions.clear();
  let clearedPositions = true;
  try {
    for (const key of Object.keys(localStorage)) if (key.startsWith(prefix)) localStorage.removeItem(key);
  } catch { storageError = true; clearedPositions = false; }
  const clearedTexts = await transact(['entries', 'texts'], 'readwrite', tx => {
    tx.objectStore('entries').clear();
    tx.objectStore('texts').clear();
    return { result: true };
  });
  return clearedPositions && clearedTexts;
}
