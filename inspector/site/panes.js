// INFO: The windows from the inspector's traced build (debug.js): trace, statistics and the sentence (an experiment).
//  A text is read into the build once and shared.
import * as debug from './debug.js';
import { $, count, element } from './dom.js';
import * as popup from './popup.js';
import { copy, question, sentenceQuestion } from './sentence/ask.js';
import * as highlights from './sentence/highlights.js';
import * as sentenceView from './sentence/sentence.js';
import { wordsOf } from './sentence/text.js';
import * as text from './text.js';

const STEP = 50;
let counted = null;

const inspectorBuild = view =>
  debug.load(bytes => { view.textContent = `Loading trace build (${(bytes / 1e6).toFixed(1)} MB) …`; });

// NOTE: Waits a frame, so the line saying a text is being read is drawn before the read holds the page.
const drawn = () => new Promise(resolve => requestAnimationFrame(() => setTimeout(resolve, 0)));

// INFO: The word opened in the text goes in the trace's box, and its link back to it.
export function setWord(written) {
  $('trace-word').value = popup.plain(written);
  $('trace-back').hidden = !written;
  $('trace-back').textContent = written ? `↑ back to ${written} in the text` : '';
}

export function resetText() {
  setWord('');
  const view = $('trace-view');
  delete view.dataset.shows;
  view.replaceChildren();
}

export async function trace() {
  const word = popup.plain($('trace-word').value.trim());
  const view = $('trace-view');
  if (!word || view.dataset.shows === word) return;
  view.dataset.shows = word;
  view.textContent = 'Loading trace build …';
  try {
    const inspector = await inspectorBuild(view);
    if (view.dataset.shows === word) view.innerHTML = inspector.word(word);
  } catch (error) {
    if (view.dataset.shows === word) view.textContent = `Could not load the trace build: ${error.message}`;
  }
}

export async function traceText() {
  const view = $('trace-view');
  const words = text.words;
  if (words.length === 0) {
    view.textContent = 'No text yet. Press i to add text.';
    return;
  }
  view.dataset.shows = '';
  view.textContent = `Reading ${count(words.length)} words …`;
  try {
    const inspector = await inspectorBuild(view);
    await drawn();
    if (words !== text.words) return;
    const groups = inspector.text(words);
    view.innerHTML = '';
    let next = 0;
    const more = element('button', 'more');
    more.type = 'button';
    const draw = () => {
      more.before(document.createRange().createContextualFragment(inspector.slice(next, STEP)));
      next += STEP;
      more.hidden = next >= groups;
      more.textContent = `${count(groups - next)} more …`;
    };
    view.append(more);
    more.addEventListener('click', draw);
    draw();
  } catch (error) {
    if (words === text.words) view.textContent = `Could not read the text into the trace build: ${error.message}`;
  }
}

export async function statistics() {
  const view = $('stats-view');
  const words = text.words;
  if (words.length === 0 || counted === words) return;
  counted = words;
  view.textContent = `Reading ${count(words.length)} words …`;
  try {
    const inspector = await inspectorBuild(view);
    await drawn();
    inspector.text(words);
    if (counted !== words) return;
    view.innerHTML = inspector.statistics();
  } catch (error) {
    counted = null;
    view.textContent = `Could not read the text into the trace build: ${error.message}`;
  }
}

// INFO: The sentence window, for word i, in the old inspector's terms: the text as one string, its words split as
//  text.js splits them, and the build's readings HTML and marks.
let guessed = null;
let oldWords = [];
let shownWord = null;

export async function sentence(i = text.last) {
  const view = $('sentence-view');
  if (text.words.length === 0) {
    view.textContent = 'No text yet. Press i to add text.';
    return;
  }
  if (i === null) {
    view.textContent = 'Open a word in the text first, then come back here.';
    return;
  }
  try {
    const inspector = await inspectorBuild(view);
    if (guessed !== text.words) {
      view.textContent = `Reading ${count(text.words.length)} words …`;
      await drawn();
      inspector.text(text.words);
      oldWords = wordsOf(text.source);
      console.assert(oldWords.length === text.words.length, 'sentence/text.js and text.js split the words alike');
      highlights.load(text.source, oldWords, inspector.marks());
      guessed = text.words;
    }
    shownWord = i;
    sentenceView.render(view, { text: text.source, words: oldWords, i, readingsHtmlOf: j => inspector.readings(j) });
  } catch (error) {
    view.textContent = `Could not read the text into the trace build: ${error.message}`;
  }
}

// NOTE: The word or its sentence as a question for a search, copied; if the browser won't copy, shown to copy by hand.
export async function ask(whole, button) {
  if (shownWord === null || guessed !== text.words) return;
  const inspector = await debug.load(() => {});
  const book = $('text-name').textContent;
  const readingsHtmlOf = j => inspector.readings(j);
  const asked = whole
    ? sentenceQuestion({ text: text.source, words: oldWords, i: shownWord, book, readingsHtmlOf,
                         isVerbal: j => highlights.isVerbal(j) })
    : question({ text: text.source, words: oldWords, i: shownWord, book, readingsHtml: readingsHtmlOf(shownWord) });
  if (await copy(asked)) {
    const label = button.textContent;
    button.textContent = 'Copied ✓';
    setTimeout(() => { button.textContent = label; }, 1500);
    return;
  }
  $('sentence-view').replaceChildren(element('pre', 'asked', asked));
}
