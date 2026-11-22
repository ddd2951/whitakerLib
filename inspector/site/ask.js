// INFO: "Copy for Google help": the selected word, or its sentence, as a question to paste into a search: one short
//  line first (plain Google search reads only its first 32 words), then the sentence, where it is in the book, the
//  library's readings, and a key to how they are written (WORDS' way is not one a search knows).

import { capitalizedInside, endsAfter } from './text.js';

const KEY = '(The readings come from whitakerLib, a version of William Whitaker\'s WORDS Latin dictionary. ' +
  'They list every form this spelling can be, not only the one meant here. "ten|ebant" marks where the stem ' +
  'ends and the ending begins. "stems" are the stems the dictionary gives for the word, as in a dictionary ' +
  'entry. Text in [ ] in a meaning is a set phrase.)';

// The words of the sentence holding word i: from after the last . ! ? before it to the next one.
export function sentenceRange(text, words, i) {
  let from = i;
  while (from > 0 && !endsAfter(text, words, from - 1)) --from;
  let to = i;
  while (to + 1 < words.length && !endsAfter(text, words, to)) ++to;
  return [from, to];
}

function sentenceOf(text, words, i) {
  const [from, to] = sentenceRange(text, words, i);
  const end = text.slice(words[to].end).match(/^[^\p{L}\n]*?[.!?]/u)?.[0] ?? '';
  return text.slice(words[from].start, words[to].end + end.length).replace(/\s+/g, ' ').trim();
}

// NOTE: A heading is a short line of its own without a full stop (Miltiades); a paragraph that starts with a number is
//  that chapter (1. Miltiades, …). Both looked for above the word; a text without them just has none.
export function placeOf(text, at) {
  const before = text.slice(0, at);
  const heading = [...before.matchAll(/^(\S[^\n.]{0,40})$/gm)].at(-1);
  const chapter = [...before.matchAll(/^(\d+)\.\s/gm)].at(-1);
  const inHeading = chapter && chapter.index > (heading?.index ?? -1);
  return [heading?.[1].trim(), inHeading ? `chapter ${chapter[1]}` : null].filter(Boolean).join(', ');
}

// The Readings view's HTML as data: each dictionary word (its kind and stems, its meaning) with its forms (stem|ending,
//  the form in words). Nothing of the view's own notes: a reader asks about the Latin, not the lookup.
export function recordsOf(html) {
  const view = document.createElement('template');
  view.innerHTML = html;
  return [...view.content.querySelectorAll('.record')].map(record => ({
    meta: record.querySelector(':scope > .meta')?.textContent.trim() ?? '',
    meaning: record.querySelector(':scope > .meaning')?.textContent.trim() ?? '',
    // NOTE: WORDS' frequency of the dictionary word, A (very common) to F (rare); '' when it gives none.
    frequency: record.querySelector(':scope > .meta.entry')?.textContent.match(/frequency (\w)/)?.[1] ?? '',
    impersonal: /\bIMPERS\b/.test(record.querySelector(':scope > .meta.entry')?.textContent ?? ''),
    forms: [...record.querySelectorAll('.reading')].map(r => ({
      split: r.querySelector('.split')?.textContent.trim() ?? '',
      words: r.querySelector('.in-words')?.textContent.trim() ?? '',
    })),
  }));
}

// NOTE: The same as lines, and how many forms. Short (for a sentence's many words): at most five dictionary words,
//  the most common of each part of speech first, so a noun is not lost behind a commoner verb (dicto: dictum 'word'
//  behind dico 'say'), then the rest by how common they are; their forms on one line.
const KEPT = 5;

// NOTE: How common a dictionary word is, lower first: WORDS' frequency (A to F, none last), and on a tie the irregular
//  verbs (WORDS' 5th to 7th conjugation: sum, volo, fero, eo …), which are among the commonest words there are
//  (vellent: volo 'want' before vello 'pluck', both A).
export const rankOf = record =>
  ('ABCDEF'.indexOf(record.frequency) + 7) % 7 * 2 + !/^verb, [5-7]th conjugation/.test(record.meta);

function readingsOf(html, short = false) {
  const lines = [];
  const all = recordsOf(html);
  const forms = all.reduce((n, r) => n + r.forms.length, 0);
  let records = all;
  if (short) {
    const ranked = [...all].sort((a, b) => rankOf(a) - rankOf(b));
    const partOf = r => r.meta.split(/[,·]/)[0].trim();
    const firsts = ranked.filter((r, k) => ranked.findIndex(o => partOf(o) === partOf(r)) === k).slice(0, KEPT);
    const kept = new Set([...firsts, ...ranked.filter(r => !firsts.includes(r))].slice(0, KEPT));
    records = ranked.filter(r => kept.has(r));
  }
  records.forEach((record, n) => {
    lines.push(`${n + 1}. ${record.meta}: "${record.meaning}"`);
    if (short)
      for (const [split, fs] of Map.groupBy(record.forms, f => f.split))
        lines.push(`   - ${split}: ${fs.map(f => f.words).join('; ')}`);
    else for (const f of record.forms) lines.push(`   - ${f.split}: ${f.words}`);
  });
  if (records.length < all.length) lines.push(`   (+ ${all.length - records.length} rarer dictionary words)`);
  return { lines, forms };
}

function aboutWord(word, html, ask, short = false) {
  const { lines, forms } = readingsOf(html, short);
  if (!lines.length) return [`whitakerLib doesn't know "${word}"; it may be a name.`];
  return [`whitakerLib finds ${forms === 1 ? 'one form' : `${forms} forms`} for "${word}".${ask}`, ...lines];
}

const where = (text, words, i, book) => `Where: ${[book, placeOf(text, words[i].start)].filter(Boolean).join(', ')}`;

// The selected word: what it means and which of its forms is meant.
export function question({ text, words, i, book, readingsHtml }) {
  const word = words[i].text;
  return [
    `Latin: what does "${word}" mean and what form is it here?`,
    '',
    `Sentence: ${sentenceOf(text, words, i)}`,
    where(text, words, i, book),
    '',
    ...aboutWord(word, readingsHtml, ' Which one applies in this sentence, and why?'),
    '',
    KEY,
  ].join('\n');
}

// NOTE: The selected word's sentence: a translation and how it is built. Only its verbs', participles' and
//  infinitives' readings go with it (a sentence's every word would bury the question), the words the library doesn't
//  know, and capitalised words it reads only as something other than a noun (Cares as careo): likely names.
export function sentenceQuestion({ text, words, i, book, readingsHtmlOf, isVerbal }) {
  const [from, to] = sentenceRange(text, words, i);
  const verbs = [];
  const unknown = [];
  for (let j = from; j <= to; ++j) {
    const html = readingsHtmlOf(j);
    const name = capitalizedInside(text, words, j) && !recordsOf(html).some(r => r.meta.startsWith('noun'));
    if (!readingsOf(html).lines.length) unknown.push(words[j].text);
    else if (name) verbs.push(...aboutWord(words[j].text, html, ' [capitalized in the text, may be a proper noun]',
                                           true), '');
    else if (isVerbal(j)) verbs.push(...aboutWord(words[j].text, html, '', true), '');
  }
  return [
    'Latin: translate this sentence and explain how it is built (main verb, subject, clauses).',
    '',
    `Sentence: ${sentenceOf(text, words, i)}`,
    where(text, words, i, book),
    '',
    ...(verbs.length ? ['Its verbs, participles and infinitives, as whitakerLib reads them:', ...verbs] : []),
    ...(unknown.length ? [`Words whitakerLib doesn't know (names, perhaps): ${unknown.join(', ')}`, ''] : []),
    KEY,
  ].join('\n');
}

// NOTE: navigator.clipboard needs https or localhost, and the page is often opened over plain http from another
//  device on the network; execCommand('copy') still works there, inside a tap. False if neither did.
export async function copy(value) {
  try {
    await navigator.clipboard.writeText(value);
    return true;
  } catch {
    const area = document.createElement('textarea');
    area.value = value;
    area.setAttribute('readonly', '');
    area.style.position = 'fixed';
    area.style.opacity = '0';
    document.body.append(area);
    area.select();
    area.setSelectionRange(0, value.length);
    const done = document.execCommand('copy');
    area.remove();
    return done;
  }
}
