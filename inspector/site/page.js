// INFO: The page: plain HTML (what it is, the books) with this on top. It reads a text in (text.js, through
//  the library, library.js), puts its numbers above it (numbers.js), opens the extension's popup on a word, and runs
//  the windows (windows.js) and the keys. The trace and statistics windows are panes.js.
import * as api from './api.js';
import { $, clock, count, element, scroller, seconds } from './dom.js';
import { initFinder } from './finder.js';
import { loadLibrary } from './library.js';
import { numbers, timing } from './numbers.js';
import * as panes from './panes.js';
import * as popup from './popup.js';
import * as shelf from './reading-library.js';
import { TEI_REVISION, teiText } from './tei.js';
import * as text from './text.js';
import { current, onLeave, onShow, showWindow, toTop } from './windows.js';

// NOTE: The books at the commit the library is measured on (data/booklist.md), not master: the numbers match, and a
//  fetched book can stay cached, since the file at a commit never changes.
const PERSEUS_COMMIT = '1b9f321c10eb85985386f135c3974a1f7f0b43b3';
const PERSEUS = `https://raw.githubusercontent.com/PerseusDL/canonical-latinLit/${PERSEUS_COMMIT}/data/`;
const PERSEUS_FILES = `https://github.com/PerseusDL/canonical-latinLit/blob/${PERSEUS_COMMIT}/data/`;
const PERSEUS_HOME = 'https://www.perseus.tufts.edu/';
const PERSEUS_REPOSITORY = 'https://github.com/PerseusDL/canonical-latinLit';
const CC_BY_SA = 'https://creativecommons.org/licenses/by-sa/4.0/';
let library;
let readingAction = 0;
let activeEntry = null;
let pendingPosition = null;
let scrollTimer;
const sample = { id: 'sample', kind: 'sample', label: $('text-name').textContent,
  name: $('text-name').textContent, source: $('passage').textContent.trim() };

function savePlace() {
  if (activeEntry && current === 'text' && !pendingPosition) shelf.savePosition(activeEntry.id, text.position());
}

function restorePlace() {
  const place = pendingPosition;
  if (!place) return;
  const stillHere = () => pendingPosition === place && current === 'text';
  text.restore(place, stillHere, () => {
    pendingPosition = null;
    if (Number.isInteger(place.last) && place.last >= 0 && place.last < text.words.length)
      open(place.last, place.popup);
  });
}

// INFO: Above a fetched book: when this browser fetched it, from which commit of Perseus, and whose it is.
function showSource(entry) {
  const line = $('text-source');
  line.hidden = entry.kind !== 'book' || !entry.commit;
  if (line.hidden) return;
  const when = new Date(entry.fetchedAt).toLocaleString(undefined, { dateStyle: 'medium', timeStyle: 'short' });
  const link = (text, href) => Object.assign(element('a', '', text), { href });
  line.replaceChildren(`Fetched by your browser on ${when} from the `, link('Perseus Digital Library', PERSEUS_HOME),
    ', ', link('canonical-latinLit', PERSEUS_REPOSITORY), ' at commit ',
    link(entry.commit.slice(0, 7), PERSEUS_FILES + entry.path), '. The text is the Perseus Digital Library’s and its ',
    'editors’, shared under ', link('CC BY-SA 4.0', CC_BY_SA), ' unless otherwise indicated.');
}

function showEntry(entry, { replaceEditor = false, resetPosition = false, ...options } = {}) {
  savePlace();
  pendingPosition = null;
  const editor = $('paste');
  if (entry.kind === 'passage' && (replaceEditor || !editor.value || editor.value === activeEntry?.source))
    editor.value = entry.source;
  activeEntry = entry;
  const place = resetPosition ? null : shelf.position(entry.id);
  if (resetPosition) shelf.savePosition(entry.id, { scroll: 0, anchor: null, last: null, popup: false });
  shelf.activate(entry.id);
  panes.resetText();
  showSource(entry);
  show(entry.source, entry.name, options);
  pendingPosition = place;
  if (current === 'text') restorePlace();
  else if (Number.isInteger(place?.last) && place.last >= 0 && place.last < text.words.length) open(place.last, false);
}

function readPassage(source, name) {
  const same = activeEntry?.kind === 'passage' && activeEntry.source === source;
  const entry = same ? activeEntry : {
    id: 'passage:' + crypto.getRandomValues(new Uint32Array(4)).join('-'), kind: 'passage', name, source,
    label: source.trim().slice(0, 90).replace(/\s+/g, ' ') + (source.trim().length > 90 ? '…' : ''),
  };
  if (!same) shelf.save(entry);
  showEntry(entry, { replaceEditor: true });
}

// NOTE: One time, from the click (or the paste) to the page painted, and its parts by name; the proof line and the
//  clock say the same.
function show(source, name, { parts = [], clicked = performance.now(), reveal = true, action = ++readingAction } = {}) {
  const lap = (part, since) => {
    const now = performance.now();
    parts.push([part, now - since]);
    return now;
  };
  let at = performance.now();
  $('current').hidden = true;
  const words = text.split(source);
  at = lap('split', at);
  const counts = library.read(words);
  at = lap('looked up', at);
  text.place(counts.known);
  $('text-name').textContent = name;
  numbers(words, counts.known, counts);
  at = lap('counted', at);
  toTop('text');
  if (!reveal) return;
  showWindow('text');
  scroller().scrollTo(0, 0);
  requestAnimationFrame(() => requestAnimationFrame(() => {
    if (action !== readingAction) return;
    lap('drawn', at);
    const total = performance.now() - clicked;
    const time = ms => (ms < 100 ? `${Math.round(ms)} ms` : seconds(ms));
    // NOTE: Browsers round timers to about a millisecond, so a short text shows no lookup time.
    const lookedUp = parts.find(([part]) => part === 'looked up')[1];
    clock(`Read in ${seconds(total)}` +
      (words.length >= 1000 ? ` · ${count(words.length)} words in ${time(lookedUp)}` : ''));
    const named = parts.map(([part, ms]) => `${part} ${time(ms)}`).join(' · ');
    timing(`${seconds(total)} from the click to the page: ${named}`);
  }));
}

// NOTE: The top line names the word x goes back to and its place as line:column, as vim's ruler does (a prose paragraph
//  is one line); the trace and api windows take the word too.
function open(i, showPopup = true) {
  const { written, answer } = text.open(i, library.word, showPopup);
  explained = { written, answer };
  const line = text.lineOf(i) + 1;
  const column = text.columnOf(i);
  $('current').replaceChildren(written, element('span', 'about', ` ${line}:${column}`));
  $('current').title = `Back to this word: line ${count(line)}, column ${column}`;
  $('current').hidden = false;
  panes.setWord(written);
  api.setWord(written);
  savePlace();
}

function backToText() {
  showWindow('text');
  const at = text.last;
  if (at === null) return;
  text.reveal(at, 'nearest').then(visible => { if (visible) open(at); });
}

function moveTo(i) {
  if (i < 0 || i >= text.words.length) return;
  text.reveal(i, 'nearest').then(visible => { if (visible) open(i); });
}

async function fetchBook(book) {
  const action = ++readingAction;
  clock(`Fetching ${book.title} …`);
  const clicked = performance.now();
  const id = `book:${PERSEUS_COMMIT}/${book.path}`;
  try {
    const cached = await shelf.load(id);
    if (action !== readingAction) return;
    if (cached?.teiRevision === TEI_REVISION) return showEntry(cached, { clicked, action });
    const response = await fetch(PERSEUS + book.path);
    if (!response.ok) throw new Error(`${response.status}`);
    const xml = await response.text();
    if (action !== readingAction) return;
    const fetched = performance.now();
    const source = teiText(xml);
    const name = `${book.author}: ${book.title}`;
    const entry = { id, kind: 'book', label: name, name, source, teiRevision: TEI_REVISION, commit: PERSEUS_COMMIT,
      path: book.path, fetchedAt: Date.now() };
    shelf.save(entry);
    showEntry(entry, {
      parts: [['fetched from Perseus', fetched - clicked], ['made text', performance.now() - fetched]],
      clicked, action, resetPosition: cached && cached.source !== source,
    });
  } catch (error) {
    if (action === readingAction) clock(`Could not fetch text: ${error.message}`);
  }
}

// NOTE: A view with no help of its own (the sentence) gets all of it.
function showHelp(all) {
  const sections = [...$('help').querySelectorAll('[data-help]')];
  all ||= !sections.some(section => section.dataset.help === current);
  for (const section of sections) section.hidden = !all && section.dataset.help !== current;
  $('help-all').textContent = all ? 'This view' : 'All help';
  $('help-all').dataset.all = String(all);
  $('help').scrollTop = 0;
}

const activeOverlay = () => document.querySelector('.overlay[open]');

// NOTE: The optional windows and the setting each needs (head.js); turned off while one is shown, back to the text.
const NEEDS = { trace: 'trace-build', stats: 'trace-build', sentence: 'experiments' };
const available = id => !NEEDS[id] || document.documentElement.hasAttribute(`data-${NEEDS[id]}`);
for (const option of new Set(Object.values(NEEDS)))
  $(option).addEventListener('change', e => {
    if (NEEDS[current] === option && !e.target.checked) showWindow('text');
  });

async function selectEntry(id) {
  if (!library) return;
  const action = ++readingAction;
  const entry = id === activeEntry?.id ? activeEntry : id === sample.id ? sample : await shelf.load(id);
  if (action !== readingAction) return;
  if (!entry) {
    $('library-storage').textContent = 'That text is no longer available.';
    $('library-storage').hidden = false;
    return;
  }
  activeOverlay()?.close();
  if (entry !== activeEntry) return showEntry(entry, { action });
  shelf.activate(id);
  showWindow('text');
  clock('Ready');
}

async function removeEntry(id) {
  if (!library) return;
  if (activeEntry?.id === id) {
    if (activeEntry.kind === 'passage' && $('paste').value === activeEntry.source) $('paste').value = '';
    activeEntry = null;
    showEntry(sample, { reveal: false });
    if (current !== 'text') showWindow(current);
  }
  await shelf.remove(id);
}

const openFinder = initFinder({
  activeId: () => activeEntry?.id,
  entries: async () => {
    const saved = await shelf.entries();
    $('library-storage').textContent = 'Some texts or reading positions could not be saved permanently.';
    $('library-storage').hidden = !shelf.storageError;
    return activeEntry?.id === sample.id || !saved.length ? [...saved, sample] : saved;
  },
  select: selectEntry,
  remove: removeEntry,
});

function openOverlay(id) {
  activeOverlay()?.close();
  if (id === 'help') showHelp(false);
  const dialog = $(id);
  dialog.showModal();
  dialog.scrollTop = 0;
  if (id === 'reading-library') openFinder();
  if (id === 'explain') showExample();
}

// INFO: Explain this answer: the real popup for the word last opened (divisa before any), and under it each kind of
//  line it shows, quoted from it, with what that kind of line means.
let explained = null;
const PATHS = [
  ['Found as written', 'found just as it is written, with nothing taken off, put back or respelled'],
  ['attached', 'an add-on was taken off and the rest looked up; what the add-on means is in italics under it'],
  ['prefix', 'a prefix was taken off and the rest looked up'],
  ['suffix', 'a suffix was taken off and the rest looked up'],
  ['contracted form', 'a shortened form from the perfect system (amasti for amavisti, amassem for amavissem), ' +
    'put back to its full form before the lookup'],
  ['another spelling', 'spelled another way in the dictionary, and read in its spelling'],
  ['adverb made', 'an adverb the dictionary does not list, made from its adjective'],
  ['Roman numeral', 'read as a Roman numeral'],
];

function showExample() {
  if (!library) return;
  const { written, answer } = explained ?? { written: 'divisa', answer: library.lookup('divisa') };
  const shown = popup.build(written, answer, { explain: false });
  $('explain-example').replaceChildren(shown);
  $('explain-word').textContent = written;
  $('explain-divisa').hidden = written.toLowerCase() !== 'divisa';
  const lines = $('explain-lines');
  lines.replaceChildren();
  const add = (quote, colour, meaning) => {
    if (!quote) return;
    lines.append(element('dt', colour, quote), element('dd', '', meaning));
  };
  const first = selector => shown.querySelector(selector)?.textContent.trim();
  const count = first('.wlib-count');
  add(count && `${written} ${count}`, 'c-word', 'The word as written, and how many ways the dictionary can read it. ' +
    'Each reading is a possibility: the popup does not know which one the sentence means. That is yours to decide.');
  const paths = [...shown.querySelectorAll('.wlib-path')].map(node => node.textContent);
  for (const path of paths) {
    const said = PATHS.filter(([words]) => path.includes(words)).map(([, meaning]) => meaning).join('; ');
    add(path, 'c-path', `How the library found it: ${said}.` +
      (paths.length > 1 && path === paths[0] ? ' A word found more than one way has a group for each.' : ''));
  }
  const part = first('.wlib-part');
  add(part, 'c-part', 'The part of speech of the dictionary word, and its class when its readings share one.' +
    (/(^|, )deponent/.test(part ?? '') ? ' A deponent verb has passive forms with an active meaning.' : '') +
    (part?.includes('semideponent') ? ' A semideponent verb is active in its present system and has passive forms ' +
      'with an active meaning in its perfect system (audeo, ausus sum).' : ''));
  add(first('.wlib-dictionary-form'), 'c-form', 'The dictionary form is how the word is listed in a Latin dictionary.');
  add(first('.wlib-meaning'), 'c-meaning', 'What the dictionary word means, in the dictionary’s own English.');
  // NOTE: The form's lines come from the one first form, so they quote one form, not pieces of several.
  const form = shown.querySelector('.wlib-form');
  const inForm = selector => form?.querySelector(selector)?.textContent.trim();
  const spelling = inForm('.wlib-spelling');
  add(spelling, 'c-form', 'The form. The dot splits the stem from the ending, as the dictionary divides the word, ' +
    'which is not always where a grammar book would.' +
    (spelling?.includes(':') ? ' Before the colon: what the form is, when that differs from the dictionary word.'
      : ''));
  const cases = inForm('.wlib-cases');
  add(inForm('.wlib-grammar'), 'c-soft', `What the form is${cases ? ', apart from its case' : ''}, and the ` +
    'dictionary word’s class when it is not beside the part of speech.');
  add(cases, '', 'The cases the form can be, for each gender and number it can be.');
  add(first('.wlib-stems'), 'c-soft', 'The dictionary’s stems for the word, which its forms are built from. ' +
    'The one not in grey is the stem this form is built on.');
  // NOTE: Each kind of note the popup shows, explained for what it says.
  for (const note of new Set([...shown.querySelectorAll('.wlib-note')].map(node => node.textContent))) {
    if (note.startsWith('read as')) add(note, 'c-soft', 'The word as the library read it: marks and letters it does ' +
      'not use are taken out. WORDS has no vowel length, so a macron cannot choose between readings.');
    else if (note === 'less likely readings were left out') add(note, 'c-soft', 'WORDS sets archaic, medieval and ' +
      'uncommon forms aside when the word also has readings that are none of those. They are not shown.');
    else if (/^(ordinal|distributive|numeral adverb):/.test(note)) add(note, 'c-soft', 'Which number this form ' +
      'means: the dictionary’s meaning is the plain number’s.');
    else if (note.startsWith('value:')) add(note, 'c-soft', 'The value of the Roman numeral.');
    else add(note, 'c-soft', 'What the add-on means, as the dictionary has it.');
  }
}

for (const dialog of document.querySelectorAll('.overlay')) {
  dialog.addEventListener('click', e => {
    if (e.target === dialog || e.target.closest('[data-close]')) dialog.close();
  });
}
$('help-all').addEventListener('click', e => showHelp(e.currentTarget.dataset.all !== 'true'));

document.addEventListener('click', e => {
  const opener = e.target.closest?.('[data-overlay]');
  const link = e.target.closest?.('a.win, #current');
  if (!opener && !link) return;
  e.preventDefault();
  if (opener) return openOverlay(opener.dataset.overlay);
  activeOverlay()?.close();
  if (link.id === 'current') backToText();
  else showWindow(link.dataset.win);
});

// NOTE: Details above the text (where a book is from, the timing, the numbers) is open in the HTML, for a wide screen
//  and for anything that reads the page without scripts; a narrow screen folds it, so the Latin comes first.
if (window.matchMedia('(max-width: 600px)').matches) $('text-details').open = false;

onLeave('text', () => { savePlace(); text.close(); });
onShow('text', restorePlace);
onShow('trace', panes.trace);
onShow('stats', panes.statistics);
onShow('api', api.show);
onShow('sentence', () => panes.sentence());
const initial = location.hash.slice(1);
const previousId = shelf.lastId();
const previousEntry = previousId && previousId !== sample.id ? shelf.load(previousId) : sample;
showWindow(document.documentElement.dataset.window);
if (['intro', 'help', 'explain', 'reading-library'].includes(initial) || (!initial && !previousId))
  openOverlay(initial || 'intro');
window.addEventListener('hashchange', () => {
  const named = location.hash.slice(1);
  if (['intro', 'help', 'explain', 'reading-library'].includes(named)) return openOverlay(named);
  if (!available(named)) return;
  if ($(named)?.classList.contains('window')) {
    activeOverlay()?.close();
    showWindow(named);
  }
});

document.addEventListener('keydown', e => {
  const dialog = activeOverlay();
  if (dialog && e.key === 'Escape') {
    e.preventDefault();
    dialog.close();
    return;
  }
  if (e.ctrlKey || e.altKey || e.metaKey || e.target.closest?.('input, textarea, select')) return;
  if ((e.key === '?' && dialog?.id !== 'help') || (e.key === 'n' && dialog?.id !== 'intro')) {
    e.preventDefault();
    openOverlay(e.key === '?' ? 'help' : 'intro');
  }
});

const books = await (await fetch('books.json')).json();
for (const [i, b] of books.entries()) {
  const option = new Option(`${b.author}: ${b.title} (${count(b.words)} words)`, i);
  if (b.title === 'Aeneid') option.selected = true;
  $('book').append(option);
}
const fetchPicked = () => fetchBook(books[$('book').value]);

library = await loadLibrary(bytes => clock(`Loading dictionary (${(bytes / 1e6).toFixed(1)} MB) …`));
if ($('explain').open) showExample();
api.init(library);
showEntry(await previousEntry ?? sample, { reveal: false });
if (current !== 'text') showWindow(current);
clock('Ready');
for (const b of document.querySelectorAll('button')) b.disabled = false;

$('trace-word').addEventListener('change', panes.trace);
$('trace-all').addEventListener('click', panes.traceText);
$('trace-example').addEventListener('click', e => {
  e.preventDefault();
  $('trace-word').value = 'simplicissimus';
  panes.trace();
});
// NOTE: The sentence view's words are buttons: a tap shows that word's sentence; its last button copies the sentence.
$('sentence-view').addEventListener('click', e => {
  const word = e.target.closest('[data-word]');
  if (word) return panes.sentence(Number(word.dataset.word));
  const ask = e.target.closest('[data-action="ask-sentence"]');
  if (ask) panes.ask(true, ask);
});
$('ask-word').addEventListener('click', e => panes.ask(false, e.currentTarget));
$('trace-back').addEventListener('click', e => {
  e.preventDefault();
  backToText();
});
$('fetch').addEventListener('click', fetchPicked);
const readBox = () => {
  if (!$('paste').value.trim()) return;
  $('paste').blur();
  readPassage($('paste').value, 'typed text');
};
$('read-paste').addEventListener('click', readBox);
$('paste').addEventListener('keydown', e => {
  if (e.key === 'Enter' && e.ctrlKey) readBox();
});
// NOTE: A tap on the word whose popup is open closes it.
$('passage').addEventListener('click', e => {
  if (!e.target.classList.contains('w')) return;
  const i = Number(e.target.dataset.i);
  if (i === text.opened) text.close();
  else open(i);
});
// NOTE: A spelling in the numbers goes to the next place it occurs after the word open now, and opens it there.
$('numbers').addEventListener('click', e => {
  const spelling = e.target.closest('.find')?.dataset.spelling;
  if (!spelling) return;
  const words = text.words;
  const from = text.opened ?? -1;
  const next = words.findIndex((w, i) => i > from && w.toLowerCase() === spelling);
  const i = next >= 0 ? next : words.findIndex(w => w.toLowerCase() === spelling);
  if (i >= 0) text.reveal(i, 'center').then(visible => { if (visible) open(i); });
});
document.addEventListener('mousedown', e => {
  if (text.opened !== null && !activeOverlay() && !e.target.closest?.('#wlib-popup, [data-overlay]') &&
      !e.target.classList.contains('w')) text.close();
});

// INFO: The keys, as Info and Help list them. None while typing in a field; space not on a button or a link either.
const WINDOWS = { x: 'text', r: 'trace', t: 'stats', p: 'api', e: 'sentence', g: 'settings', i: 'input' };
const ARROWS = { ArrowLeft: 'back', ArrowRight: 'on', ArrowDown: 'down', ArrowUp: 'up' };
const LAYOUTS = {
  arrows: {},
  vim: { h: 'back', l: 'on', j: 'down', k: 'up' },
  wasd: { a: 'back', d: 'on', s: 'down', w: 'up' },
};
let layout = 'arrows';
try {
  layout = LAYOUTS[localStorage.getItem('keys')] ? localStorage.getItem('keys') : 'arrows';
} catch {
  // NOTE: No storage: the arrows alone
}
for (const radio of document.querySelectorAll('input[name="keys"]')) {
  radio.checked = radio.value === layout;
  radio.addEventListener('change', () => {
    layout = radio.value;
    try {
      localStorage.setItem('keys', layout);
    } catch {
      // NOTE: Not kept: it holds until the page is closed
    }
  });
}

document.addEventListener('keydown', e => {
  if (e.defaultPrevented || activeOverlay()) return;
  if (e.key === 'Escape') return text.close();
  if (e.ctrlKey || e.altKey || e.metaKey || e.target.closest?.('input, textarea, select')) return;
  if (e.key === ' ') {
    if (e.target.closest?.('button, summary, a')) return;
    e.preventDefault();
    return openOverlay('reading-library');
  }
  if (e.key === 'f') {
    e.preventDefault();
    return fetchPicked();
  }
  if (WINDOWS[e.key]) {
    if (!available(WINDOWS[e.key])) return;
    e.preventDefault();
    return showWindow(WINDOWS[e.key]);
  }
  // NOTE: The arrows only with a word open: closed, they are the page's, for scrolling.
  const at = ARROWS[e.key] ? text.opened : text.opened ?? text.last;
  const move = ARROWS[e.key] ?? LAYOUTS[layout][e.key];
  if (current !== 'text' || at === null || !move) return;
  e.preventDefault();
  if (move === 'back') moveTo(at - 1);
  else if (move === 'on') moveTo(at + 1);
  else moveTo(text.wordOnRow(at, move === 'down' ? 1 : -1));
});
document.addEventListener('paste', e => {
  if (activeOverlay() || e.target.closest?.('input, textarea, select')) return;
  const pasted = e.clipboardData.getData('text/plain');
  if (pasted.trim()) {
    e.preventDefault();
    readPassage(pasted, 'pasted text');
  }
});

// NOTE: The page scrolls, or on a touch screen the box in it (dom.js): both are heard.
const scrolled = () => {
  clearTimeout(scrollTimer);
  scrollTimer = setTimeout(savePlace, 120);
};
window.addEventListener('scroll', scrolled, { passive: true });
$('scroll').addEventListener('scroll', scrolled, { passive: true });
window.addEventListener('pagehide', savePlace);
document.addEventListener('visibilitychange', () => { if (document.hidden) savePlace(); });
$('clear-library').addEventListener('click', async () => {
  if (!window.confirm('Delete all saved books, passages, and positions from this browser?')) return;
  const action = ++readingAction;
  activeEntry = null;
  pendingPosition = null;
  $('paste').value = '';
  const cleared = await shelf.clear();
  if (action === readingAction) showEntry(sample, { reveal: false, action });
  $('library-settings-status').textContent = cleared ? 'Library cleared.'
    : 'Could not verify that your library was cleared.';
});
