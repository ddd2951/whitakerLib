// INFO: The text on the page, held in chunks of CHUNK lines. A chunk is plain text until it comes near the screen (or
//  a key or a click needs one of its words); then it gets a span a word, for the popup, the keys and the red of words
//  with no reading. The browser lays out and draws only the chunks on screen (content-visibility: auto, style.css), so
//  a book costs about what a page of it costs, and Ctrl+F still finds every word.
import { $, element, readingTop, scroller } from './dom.js';
import * as popup from './popup.js';

const CHUNK = 64;
export const WORD = /\p{L}[\p{L}\p{M}]*/gu;
// NOTE: Most words are plain letters already: popup.plain (the extension's folding) only for the rest.
const spelling = w => (/^[A-Za-z]+$/.test(w) ? w : popup.plain(w));

// INFO: The text as given, its words as the library reads them, and the words open now and opened last.
export let source = '';
export let words = [];
export let opened = null;
export let last = null;
// NOTE: A flag a word, '1' if it has a reading: what makes a word red when its chunk is built.
let known = '';

let lines = [];
let lineFirst = [];
let spans = [];
let chunks = [];
let built = [];
let passage = null;
let watch = null;

// INFO: A new text, split into lines and words, its chunks made but not yet on the page: the words go to the library
//  first, and place() puts the text up with what it found.
export function split(text) {
  close();
  last = null;
  watch?.disconnect();
  source = text;
  lines = text.split('\n');
  lineFirst = new Array(lines.length);
  words = [];
  for (let l = 0; l < lines.length; ++l) {
    lineFirst[l] = words.length;
    for (const m of lines[l].matchAll(WORD)) words.push(spelling(m[0]));
  }
  spans = new Array(words.length);
  passage = document.createDocumentFragment();
  chunks = [];
  built = [];
  for (let c = 0; c * CHUNK < lines.length; ++c) {
    const chunk = element('div', 'chunk', lines.slice(c * CHUNK, (c + 1) * CHUNK).join('\n'));
    chunk.dataset.c = c;
    chunks.push(chunk);
    passage.append(chunk);
  }
  return words;
}

export function place(flags) {
  known = flags;
  $('passage').replaceChildren(passage);
  watch = new IntersectionObserver(entries => {
    for (const e of entries) if (e.isIntersecting) spanify(Number(e.target.dataset.c));
  }, { rootMargin: '1500px 0px' });
  for (const chunk of chunks) watch.observe(chunk);
}

function spanify(c) {
  if (built[c]) return;
  built[c] = true;
  const part = document.createDocumentFragment();
  for (let l = c * CHUNK; l < Math.min(lines.length, (c + 1) * CHUNK); ++l) {
    const div = element('div', 'line');
    const line = lines[l];
    let at = 0;
    let i = lineFirst[l];
    for (const m of line.matchAll(WORD)) {
      if (m.index > at) div.append(line.slice(at, m.index));
      const span = element('span', known.charCodeAt(i) === 48 ? 'w unknown' : 'w', m[0]);
      if (i === opened) span.classList.add('on');
      span.dataset.i = i;
      div.append(span);
      spans[i++] = span;
      at = m.index + m[0].length;
    }
    if (at < line.length) div.append(line.slice(at));
    part.append(div);
  }
  chunks[c].replaceChildren(part);
  chunks[c].classList.add('built');
}

// NOTE: The line a word is on: the last line starting at or before it (an empty line starts where the next one does).
export function lineOf(i) {
  let lo = 0;
  let hi = lines.length - 1;
  while (lo < hi) {
    const mid = (lo + hi + 1) >> 1;
    if (lineFirst[mid] <= i) lo = mid;
    else hi = mid - 1;
  }
  return lo;
}

// INFO: The column word i starts at in its line, from 1, in characters, as vim's ruler counts it.
export function columnOf(i) {
  const l = lineOf(i);
  const match = [...lines[l].matchAll(WORD)][i - lineFirst[l]];
  return [...lines[l].slice(0, match.index)].length + 1;
}

function spanAt(i) {
  spanify(Math.floor(lineOf(i) / CHUNK));
  return spans[i];
}

// NOTE: A word far off is scrolled to twice: the first time, the chunks on the way still count at their estimated
//  height; a frame later they are laid out at their own, and the second scroll lands on the word.
export function reveal(i, block) {
  return new Promise(resolve => {
    const span = spanAt(i);
    span.scrollIntoView({ block });
    requestAnimationFrame(() => requestAnimationFrame(() => {
      if (!span.isConnected || document.documentElement.dataset.window !== 'text') return resolve(false);
      span.scrollIntoView({ block });
      resolve(true);
    }));
  });
}

function firstBetween(lo, hi, test) {
  while (lo < hi) {
    const mid = (lo + hi) >>> 1;
    if (test(mid)) hi = mid;
    else lo = mid + 1;
  }
  return lo;
}


// NOTE: A visible word anchors the reading place when the same lines wrap differently after a resize or reload.
export function position() {
  const place = { scroll: scroller().scrollTop, last, popup: opened !== null };
  const top = readingTop();
  if (!place.scroll || $('passage').getBoundingClientRect().top > top) return place;
  const chunk = firstBetween(0, chunks.length, i => chunks[i].getBoundingClientRect().bottom > top);
  if (chunk === chunks.length) return place;
  spanify(chunk);
  const end = lineFirst[(chunk + 1) * CHUNK] ?? words.length;
  const i = firstBetween(lineFirst[chunk * CHUNK], end, j => spans[j].getBoundingClientRect().bottom > top);
  if (i === end) return place;
  return { ...place, anchor: i, offset: spans[i].getBoundingClientRect().top - top };
}

export function restore(place, isCurrent, ready) {
  const anchored = Number.isInteger(place.anchor) && place.anchor >= 0 && place.anchor < words.length;
  const span = anchored ? spanAt(place.anchor) : null;
  const scroll = () => {
    const y = span ? scroller().scrollTop + span.getBoundingClientRect().top - readingTop() - place.offset
      : place.scroll;
    scroller().scrollTo(0, y || 0);
  };
  scroll();
  requestAnimationFrame(() => requestAnimationFrame(() => {
    if (!isCurrent()) return;
    scroll();
    ready();
  }));
}

export function close() {
  popup.close();
  spans[opened]?.classList.remove('on');
  opened = null;
}

// INFO: Opens word i's readings in the popup; gives the word as written and the library's answer (library.word).
export function open(i, word, showPopup = true) {
  close();
  const span = spanAt(i);
  let answer;
  try {
    answer = word(i);
  } catch (error) {
    answer = { word: span.textContent, error: `whitakerLib: ${error.message}` };
  }
  last = i;
  if (showPopup) {
    const rect = span.getBoundingClientRect();
    if (rect.bottom > readingTop() && rect.top < window.innerHeight) {
      span.classList.add('on');
      opened = i;
      popup.show(span.textContent, answer, rect);
    }
  }
  return { written: span.textContent, answer };
}

// NOTE: The word on the next row down (or up) as laid out, nearest the open word's middle; looked for among the next
//  400 words, more than any row holds, so a whole book is never measured.
export function wordOnRow(from, step) {
  const at = spanAt(from).getBoundingClientRect();
  const middle = (at.left + at.right) / 2;
  let row = null;
  let best = null;
  for (let i = from + step, n = 0; i >= 0 && i < words.length && n < 400; i += step, ++n) {
    const r = spanAt(i).getBoundingClientRect();
    if (r.height === 0 || (step > 0 ? r.top <= at.top + 2 : r.top >= at.top - 2)) continue;
    if (row === null) row = r.top;
    if (Math.abs(r.top - row) > 2) break;
    const distance = Math.abs((r.left + r.right) / 2 - middle);
    if (best === null || distance < best.distance) best = { i, distance };
  }
  return best?.i ?? from;
}
