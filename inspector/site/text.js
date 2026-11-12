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

// NOTE: One fragment, not replaceChildren(...parts): Firefox takes at most 500,000 arguments to a call, and a long
//  book has about two parts a word (Livy over a million).
export function renderPassage(container, text, words, known) {
  const passage = document.createDocumentFragment();
  let at = 0;
  words.forEach((o, i) => {
    if (o.start > at) passage.append(text.slice(at, o.start));
    const span = document.createElement('span');
    span.className = known[i] === '0' ? 'w unknown' : 'w';
    span.dataset.i = i;
    span.textContent = o.text;
    passage.append(span);
    at = o.end;
  });
  if (at < text.length) passage.append(text.slice(at));
  container.replaceChildren(passage);
  return [...container.querySelectorAll('.w')];
}
