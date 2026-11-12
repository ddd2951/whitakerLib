// INFO: The inspector: paste or open a text, select a word or a run of words, and see the result in each view. The
//  selection and the view are chosen independently; changing one keeps the other. Nothing is stored or sent.
// TODO: post 0.4.0 The page's actions as calls (load, select, view, search), for an agent to drive the site.
// NOTE: The inspector will mostly be reworked after 0.4.0. Build nothing against the page or its calls, and depend
//  on nothing in them.
import { loadLibrary } from './library.js';
import { renderPassage, withoutGutenberg, wordsOf } from './text.js';
import { render, VIEWS } from './views.js';

const $ = id => document.getElementById(id);
const PROMPT = 'Ctrl+V anywhere to inspect a Latin text, or type it here and press Enter';

let library;
let words = [];
let spans = [];
let anchor = 0;
let focus = 0;
let marked = [0, -1];
let view = 'readable';
let binding = 0;
let matches = [];
let dragging = false;
let sampling = null;
let drawn = 0;
let loading = false;

const range = () => [Math.min(anchor, focus), Math.max(anchor, focus)];
const wholeText = () => range()[0] === 0 && range()[1] === words.length - 1;

// NOTE: Marking a whole book word by word took about 200 ms, so the whole text is one class on the passage and other
//  ranges change only the words that entered or left them.
function mark() {
  const [from, to] = range();
  const all = from === 0 && to === spans.length - 1 && from !== to;
  $('passage').classList.toggle('all', all);
  const [first, last] = all ? [0, -1] : [from, to];
  for (let i = marked[0]; i <= marked[1]; ++i) if (i < first || i > last) spans[i].classList.remove('inrange');
  for (let i = first; i <= last; ++i) if (i < marked[0] || i > marked[1]) spans[i].classList.add('inrange');
  spans[marked[2]]?.classList.remove('selected');
  spans[focus].classList.add('selected');
  marked = [first, last, focus];
  return all;
}

function show(started = performance.now()) {
  if (!words.length) return;
  if (!mark()) spans[focus].scrollIntoView({ block: 'nearest' });
  const [from, to] = range();
  $('word').textContent = from === to ? words[from].text : `${words[from].text} … ${words[to].text}`;
  document.querySelectorAll('#views button').forEach(b => b.classList.toggle('on', b.dataset.view === view));
  render(view, library.select(from, to), library, $('view'));
  const current = ++drawn;
  requestAnimationFrame(() => requestAnimationFrame(() => {
    if (current !== drawn) return;
    const ms = `${Math.round(performance.now() - started)} ms`;
    const paint = () => { const p = $('view').querySelector('.paint'); if (p) p.textContent = ms; };
    paint();
    if (view !== 'statistics' || dragging) return;
    setTimeout(() => {
      if (current !== drawn || !library.time()) return;
      render(view, 0, library, $('view'));
      paint();
    }, 0);
  }));
}

function select(i, extend = false, started = performance.now()) {
  if (!words.length) return;
  focus = Math.max(0, Math.min(words.length - 1, i));
  if (!extend) anchor = focus;
  show(started);
  countMatches();
}

function selectAll() {
  if (!words.length) return;
  anchor = 0;
  focus = words.length - 1;
  show();
}

function setView(v) {
  view = v;
  show();
}

function setBinding(b) {
  binding = b;
  library.binding(b);
  if (view === 'api') show();
  else setView('api');
}

function find(query) {
  $('search').value = query;
  runSearch();
}

function runSearch() {
  spans.forEach(s => s.classList.remove('match'));
  const query = $('search').value.trim();
  const found = words.length && query ? library.search(query) : '';
  matches = [];
  for (let i = 0; i < found.length; ++i) if (found[i] === '1') matches.push(i);
  for (const i of matches) spans[i].classList.add('match');
  countMatches();
}

function countMatches() {
  const query = $('search').value.trim();
  const at = matches.indexOf(focus);
  $('search-count').textContent = !query ? '' : matches.length === 0 ? 'no match' :
    at >= 0 ? `${at + 1} of ${matches.length}` : `${matches.length} matches`;
}

function stepMatch(direction) {
  if (matches.length === 0) return;
  select(direction > 0 ? matches.find(i => i > focus) ?? matches[0]
                       : matches.findLast(i => i < focus) ?? matches.at(-1));
}

// NOTE: The library takes the new text before the passage is drawn. If anything after that fails, both are emptied,
//  so no word on screen shows another word's readings, and the text box says what failed.
// NOTE: A book takes seconds, so the text box says so first, drawn before the work starts; a text that comes in
//  meanwhile is left alone.
async function start(source) {
  if (loading) return;
  loading = true;
  try {
    const text = withoutGutenberg(source);
    const next = wordsOf(text);
    $('source').placeholder = `Looking up ${next.length.toLocaleString('en')} words …`;
    await new Promise(resolve => requestAnimationFrame(() => setTimeout(resolve, 0)));
    words = next;
    const known = library.load(words.map(o => o.spelling));
    spans = renderPassage($('passage'), text, words, known);
    $('source').placeholder = PROMPT;
  } catch (error) {
    words = [];
    spans = [];
    library.load([]);
    $('passage').replaceChildren();
    $('word').textContent = '';
    $('view').replaceChildren();
    $('source').placeholder = `Could not take that text: ${error.message}`;
  }
  loading = false;
  anchor = focus = 0;
  marked = [0, -1];
  sampling = null;
  runSearch();
  if (words.length) select(0);
  if ($('sample').checked && words.length) startSampling();
}

function startSampling() {
  const rounds = Math.max(1, Math.min(1000, Number($('sample-count').value) || 1));
  const mine = sampling = {};
  library.sampleStart(rounds);
  let round = 0;
  let shown = 0;
  (async () => {
    while (round < rounds) {
      await new Promise(resolve => setTimeout(resolve, 0));
      if (sampling !== mine) return;
      const done = library.sample();
      if (done === round) continue;
      round = done;
      if (view === 'statistics' && wholeText() && (round === rounds || performance.now() - shown > 500)) {
        shown = performance.now();
        show();
      }
    }
  })();
}

const wordAt = target => target.classList?.contains('w') ? Number(target.dataset.i) : null;

function wirePassage() {
  const passage = $('passage');
  let press = null;
  let pressed = false;
  let pending = false;
  let pointer = 'mouse';
  passage.addEventListener('pointerdown', e => {
    const i = wordAt(e.target);
    pointer = e.pointerType;
    if (i === null) return;
    if (e.pointerType === 'mouse') {
      if (e.button !== 0) return;
      e.preventDefault();
      dragging = true;
      select(i, e.shiftKey);
      return;
    }
    pressed = false;
    press = setTimeout(() => {
      pressed = true;
      select(i, true);
    }, 450);
  });
  passage.addEventListener('pointerover', e => {
    const i = wordAt(e.target);
    if (!dragging || i === null || i === focus) return;
    focus = i;
    if (pending) return;
    pending = true;
    requestAnimationFrame(() => {
      pending = false;
      show();
    });
  });
  const cancel = () => clearTimeout(press);
  passage.addEventListener('pointermove', e => { if (e.pointerType !== 'mouse') cancel(); });
  passage.addEventListener('pointercancel', cancel);
  document.addEventListener('pointerup', e => {
    cancel();
    if (!dragging) return;
    dragging = false;
    show(e.timeStamp);
  });
  passage.addEventListener('click', e => {
    const i = wordAt(e.target);
    if (i === null || pointer === 'mouse') return;
    if (pressed) {
      pressed = false;
      return;
    }
    select(i, e.shiftKey, e.timeStamp);
  });
  passage.addEventListener('contextmenu', e => { if (wordAt(e.target) !== null) e.preventDefault(); });
}

function take(source) {
  $('source').value = '';
  $('source').blur();
  if (source.trim()) start(source);
}

function wire() {
  // NOTE: A paste is a new text only in the text box or outside any field; every other field takes it as its own.
  document.addEventListener('paste', e => {
    if (e.target.closest('input, textarea') && e.target.id !== 'source') return;
    e.preventDefault();
    take(e.clipboardData.getData('text'));
  });
  $('source').addEventListener('keydown', e => {
    if (e.key === 'Enter' && !e.shiftKey) take($('source').value);
    else if (e.key === 'Escape') $('source').blur();
    else return;
    e.preventDefault();
  });
  // NOTE: Several files are one text, in the order the browser lists them, a blank line between; each loses its
  //  Project Gutenberg header and footer on its own.
  $('open-file').addEventListener('change', async e => {
    const files = [...e.target.files];
    if (!files.length) return;
    e.target.value = '';
    start((await Promise.all(files.map(file => file.text()))).map(withoutGutenberg).join('\n\n'));
  });
  $('sample').addEventListener('change', () => {
    if ($('sample').checked && words.length) startSampling();
    else if (!$('sample').checked) sampling = null;
  });
  wirePassage();
  $('views').addEventListener('click', e => { if (e.target.dataset.view) setView(e.target.dataset.view); });
  $('view').addEventListener('click', e => {
    const b = e.target.closest('[data-find]');
    if (b) find(b.dataset.find);
    const k = e.target.closest('[data-binding]');
    if (k) setBinding(Number(k.dataset.binding));
  });
  $('select-all').addEventListener('click', selectAll);
  $('search').addEventListener('input', runSearch);
  $('search-prev').addEventListener('click', () => stepMatch(-1));
  $('search-next').addEventListener('click', () => stepMatch(1));
  $('search').addEventListener('keydown', e => {
    if (e.key === 'Enter') {
      stepMatch(1);
      $('search').blur();
    } else if (e.key === 'ArrowDown') stepMatch(1);
    else if (e.key === 'ArrowUp') stepMatch(-1);
    else if (e.key === 'Escape') $('search').blur();
    else return;
    e.preventDefault();
  });

  document.addEventListener('keydown', e => {
    if (e.target.closest('input, textarea') || e.ctrlKey || e.metaKey || e.altKey) return;
    if (e.key === 'o' || e.key === 't') {
      if (e.key === 'o') $('open-file').click();
      else $('sample').click();
      e.preventDefault();
      return;
    }
    if (!words.length) return;
    const v = VIEWS.indexOf(view);
    switch (e.key) {
      case 'ArrowLeft': select(focus - 1, e.shiftKey, e.timeStamp); break;
      case 'ArrowRight': select(focus + 1, e.shiftKey, e.timeStamp); break;
      case ',': setView(VIEWS[(v + VIEWS.length - 1) % VIEWS.length]); break;
      case '.': setView(VIEWS[(v + 1) % VIEWS.length]); break;
      case 'ArrowUp': stepMatch(-1); break;
      case 'ArrowDown': stepMatch(1); break;
      case '1': case '2': case '3': case '4': setView(VIEWS[Number(e.key) - 1]); break;
      case 'b': setBinding((binding + 1) % 3); break;
      case '/': $('search').focus(); break;
      case 'a': selectAll(); break;
      case 'Escape': document.activeElement?.blur(); break;
      default: return;
    }
    e.preventDefault();
  });
}

try {
  library = await loadLibrary(bytes => {
    $('source').placeholder = `Loading the dictionary, once: ${(bytes / 1e6).toFixed(1)} MB …`;
  });
  wire();
  $('source').disabled = false;
  $('source').placeholder = PROMPT;
  $('source').focus();
} catch (error) {
  $('source').placeholder = `Could not load the dictionary: ${error.message}`;
}
