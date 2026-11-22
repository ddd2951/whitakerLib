// INFO: The supplied text as words, and the passage that shows them. The library looks the words up; here each word
//  keeps its place in the text.

// NOTE: The library reads ASCII letters; accents and macrons are taken off for the lookup only.
const plain = word => word.normalize('NFD').replace(/\p{M}/gu, '');

export function withoutGutenberg(text) {
  const start = text.match(/^\*\*\* ?START OF (THE|THIS) PROJECT GUTENBERG.*$/im);
  const end = text.match(/^\*\*\* ?END OF (THE|THIS) PROJECT GUTENBERG.*$/im);
  if (!start || !end || end.index < start.index) return text;
  return text.slice(start.index + start[0].length, end.index).trim();
}

export const wordsOf = text => [...text.matchAll(/\p{L}+/gu)].map(match =>
  ({ text: match[0], spelling: plain(match[0]), start: match.index, end: match.index + match[0].length }));

// Whether the gap after word i ends a stretch of the text. A full stop after a capital of up to three letters is a
//  Roman first name (M., T., Cn.), not the end of a sentence.
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

// NOTE: One fragment, not replaceChildren(...parts): Firefox takes at most 500,000 arguments to a call, and a long
//  book has about two parts a word (Livy over a million).
export function renderPassage(container, text, words, known, kinds) {
  const passage = document.createDocumentFragment();
  let at = 0;
  words.forEach((o, i) => {
    if (o.start > at) passage.append(text.slice(at, o.start));
    const span = document.createElement('span');
    span.className = (known[i] === '0' ? 'w unknown' : 'w') + kinds[i];
    span.dataset.i = i;
    span.textContent = o.text;
    passage.append(span);
    at = o.end;
  });
  if (at < text.length) passage.append(text.slice(at));
  container.replaceChildren(passage);
  return [...container.querySelectorAll('.w')];
}
