// INFO: Word offsets, sentence boundaries and capitalisation for the Sentence view and its copy buttons.
import { WORD } from '../text.js';

// NOTE: The library reads ASCII letters; accents and macrons are taken off for the lookup only.
const plain = word => word.normalize('NFD').replace(/\p{M}/gu, '');

export const wordsOf = text => [...text.matchAll(WORD)].map(match =>
  ({ text: match[0], spelling: plain(match[0]), start: match.index, end: match.index + match[0].length }));

// INFO: Whether the gap after word i ends a stretch of the text. A full stop after a capital of up to three letters is
//  a Roman first name (M., T., Cn.), not the end of a sentence.
export function endsAfter(text, words, i, marks = /[.!?]/) {
  const gap = i + 1 < words.length ? text.slice(words[i].end, words[i + 1].start) : '.';
  if (!marks.test(gap)) return false;
  return !(gap.trimStart().startsWith('.') && /^\p{Lu}\p{Ll}{0,2}$/u.test(words[i].text) &&
           !marks.test(gap.trimStart().slice(1)));
}

// NOTE: Whether word i is capitalised where only a name would be: inside a sentence, or, in a text that starts most
//  sentences in lowercase (Perseus' editions do), anywhere but at the start of a paragraph. Such a word is a name
//  (Cares, a people), whatever the library reads (careo, 'lack').
let starts = { text: null, lowercase: false };
export function capitalizedInside(text, words, i) {
  if (!/^\p{Lu}/u.test(words[i].text)) return false;
  const paragraph = i === 0 || /\n\s*(\d+\.\s*)?$/.test(text.slice(words[i - 1].end, words[i].start));
  if (paragraph) return false;
  if (!endsAfter(text, words, i - 1)) return true;
  if (starts.text !== text) {
    let lower = 0;
    let all = 0;
    for (let j = 1; j < words.length; ++j)
      if (endsAfter(text, words, j - 1)) {
        ++all;
        lower += /^\p{Ll}/u.test(words[j].text);
      }
    starts = { text, lowercase: lower * 2 > all };
  }
  return starts.lowercase;
}
