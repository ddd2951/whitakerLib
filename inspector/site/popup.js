// INFO: The Firefox extension's popup (whitakerLib-firefox 0.1.1), with its readings grouped by how the library found
//  them and a form's cases on one line, read from the text the page has read (library.word, ../page.cpp).

import { readingTop, scroller } from './dom.js';

// TODO: Until the library takes them: long marks and ligatures folded to the plain letters it reads, amō to
//  amo, cæsar to caesar. WORDS has no vowel length, so the marks cannot choose a reading; the popup says so.
export const plain = word =>
  word.normalize('NFD').replace(/\p{M}/gu, '').replace(/æ/g, 'ae').replace(/Æ/g, 'Ae').replace(/œ/g, 'oe')
    .replace(/Œ/g, 'Oe');
let popup = null;

export function close() {
  popup?.remove();
  popup = null;
}

function element(tag, className, text) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (text) node.textContent = text;
  return node;
}

// INFO: How a reading was found, in plain words, from the library's route, contraction, respelling and add-ons
//  (../page.cpp): the popup's readings are grouped by it, the plain dictionary lookup first.
const DIRECT = 'Found as written';

function addonPhrase(addon) {
  if (addon.kind === 'prefix' || addon.kind === 'tickon') return `the prefix ${addon.spelling}-`;
  if (addon.kind === 'suffix') return `the suffix -${addon.spelling}-`;
  return `-${addon.spelling} attached`;
}

function pathOf(reading) {
  const parts = [];
  const full = reading.stem + reading.ending;
  if (reading.numeral) parts.push('a Roman numeral');
  if (reading.syncope && reading.respelling) parts.push(`read as ${full}, a contracted form and another spelling`);
  else if (reading.syncope) parts.push(`read as ${full}, a contracted form`);
  else if (reading.respelling) parts.push(`read as ${full}, another spelling of it`);
  if (reading.adverb) parts.push('an adverb made from an adjective');
  if (reading.addons.length > 0) parts.push(`with ${reading.addons.map(addonPhrase).join(' and ')}`);
  const label = parts.join('; ');
  return label ? label.charAt(0).toUpperCase() + label.slice(1) : DIRECT;
}

// INFO: The readings by how they were found, then by part of speech, then by meaning and dictionary form,
//  each in the library's order.
function groups(readings) {
  const byPath = new Map([[DIRECT, []]]);
  for (const reading of readings) {
    const path = pathOf(reading);
    if (!byPath.has(path)) byPath.set(path, []);
    byPath.get(path).push(reading);
  }
  if (byPath.get(DIRECT).length === 0) byPath.delete(DIRECT);
  return byPath;
}

function dictionaryForm(reading) {
  // NOTE: WORDS puts its part and class after two spaces; the popup already gives them in the reader's words.
  return reading.dictionaryForm.split('  ', 1)[0];
}

function sections(readings) {
  const byPart = new Map();
  for (const reading of readings) {
    if (!byPart.has(reading.dictionaryPart)) byPart.set(reading.dictionaryPart, new Map());
    const meanings = byPart.get(reading.dictionaryPart);
    const key = JSON.stringify([reading.meaning, dictionaryForm(reading)]);
    if (!meanings.has(key)) meanings.set(key, []);
    meanings.get(key).push(reading);
  }
  return byPart;
}

// INFO: One line a form; a form's readings that differ only in case and agreement are one line, each gender and
//  number under it with its cases: divis·a perfect, passive / feminine, singular: nominative, vocative, ablative.
//  A reading with no case (a verb's) keeps its whole grammar on its line.
function forms(readings, classInHead) {
  const byForm = new Map();
  for (const reading of readings) {
    let spelling = reading.ending ? `${reading.stem}·${reading.ending}` : reading.stem;
    if (reading.part !== reading.dictionaryPart) spelling = `${reading.part}: ${spelling}`;
    const cls = classInHead ? '' : reading.class;
    const about = reading.case ? reading.rest : reading.grammar;
    const key = [spelling, about, cls, reading.stemKey, reading.numeral].join('|');
    if (!byForm.has(key)) byForm.set(key, { spelling, about, cls, reading, readings: [], cases: new Map() });
    byForm.get(key).readings.push(reading);
    if (reading.case) {
      const cases = byForm.get(key).cases;
      if (!cases.has(reading.agreement)) cases.set(reading.agreement, []);
      cases.get(reading.agreement).push(reading.case);
    }
  }
  return [...byForm.values()];
}

function formNode({ spelling, about, cls, reading, cases }) {
  const node = element('div', 'wlib-form');
  node.append(element('span', 'wlib-spelling', spelling));
  const said = [about, cls].filter(Boolean).join(' · ');
  if (said) node.append(element('span', 'wlib-grammar', said));
  for (const [agreement, list] of cases) {
    const line = element('div', 'wlib-cases');
    line.append(element('span', 'wlib-agreement', agreement ? `${agreement}: ` : ''), list.join(', '));
    node.append(line);
  }
  if (reading.numeral) node.append(element('div', 'wlib-note', `value: ${reading.numeral}`));
  const which = numberSaid(reading);
  if (which) node.append(element('div', 'wlib-note', which));
  return node;
}

// INFO: Which number a numeral's form means, since the dictionary's meaning is the cardinal's: tertiam "3rd".
function numberSaid({ number, sort }) {
  if (!number) return '';
  const nth = number % 100 >= 11 && number % 100 <= 13 ? 'th' : ['th', 'st', 'nd', 'rd'][number % 10] ?? 'th';
  return { ordinal: `ordinal: ${number}${nth}`, distributive: `distributive: ${number} each`,
    adverbial: `numeral adverb: ${number} times` }[sort] ?? '';
}

function stemsLine(reading) {
  const node = element('div', 'wlib-stems', 'stems ');
  reading.stems.forEach((stem, i) => {
    if (i > 0) node.append(', ');
    node.append(element(stem.key === reading.stemKey ? 'b' : 'span', '', stem.spelling));
  });
  return node;
}

// INFO: The popup for a word, built but not placed: show() places it by the word, and Explain this answer shows one
//  as its example ({ explain: false }: without the link to itself).
export function build(written, answer, { explain = true } = {}) {
  const readings = answer.readings ?? [];
  const popup = element('div', 'wlib-popup');
  const head = element('div', 'wlib-word', written);
  if (readings.length > 0)
    head.append(element('span', 'wlib-count', readings.length === 1 ? '1 reading' : `${readings.length} readings`));
  popup.append(head);
  if (answer.word && answer.word !== written) {
    const marked = /\p{M}/u.test(written.normalize('NFD'));
    const why = marked ? ': macron marks are ignored during lookup' : '';
    popup.append(element('div', 'wlib-note', `read as “${answer.word}”${why}`));
  }
  if (answer.error) popup.append(element('div', 'wlib-note', answer.error));
  else if (readings.length === 0) popup.append(element('div', 'wlib-note', 'No readings found'));

  for (const [path, found] of groups(readings)) {
    const group = element('div', 'wlib-group');
    group.append(element('div', 'wlib-path', path));
    const meanings = new Set(found.flatMap(r => r.addons.map(a => a.meaning)));
    for (const meaning of meanings) group.append(element('div', 'wlib-note', meaning));
    for (const [part, byMeaning] of sections(found)) {
      const section = element('div', 'wlib-section');
      // NOTE: The class goes up beside the part when all its readings share it (Verb · 3rd conjugation).
      const all = [...byMeaning.values()].flat();
      const shared = all.every(r => r.class === all[0].class) ? all[0].class : '';
      const name = part.charAt(0).toUpperCase() + part.slice(1);
      section.append(element('div', 'wlib-part', shared ? `${name} · ${shared}` : name));
      for (const list of byMeaning.values()) {
        const principalParts = dictionaryForm(list[0]);
        if (principalParts) section.append(element('div', 'wlib-spelling wlib-dictionary-form', principalParts));
        section.append(element('div', 'wlib-meaning', list[0].meaning));
        const stems = new Set();
        for (const form of forms(list, shared !== '')) {
          section.append(formNode(form));
          const key = `${form.reading.stemKey} ${form.reading.stems.map(stem => stem.spelling).join()}`;
          if (form.reading.stems.length > 0 && !stems.has(key)) {
            stems.add(key);
            section.append(stemsLine(form.reading));
          }
        }
      }
      group.append(section);
    }
    popup.append(group);
  }
  // TODO: Why readings are left out, said to the reader, and which.
  if (answer.trimmed) popup.append(element('div', 'wlib-note', 'less likely readings were left out'));
  // NOTE: The page's own, not the extension's: what each line means, in an overlay (page.js opens [data-overlay]).
  if (explain) {
    const link = Object.assign(element('a', 'wlib-explain', 'Explain this answer'), { href: '#explain' });
    link.dataset.overlay = 'explain';
    popup.append(link);
  }
  return popup;
}

export function show(written, answer, rect) {
  close();
  popup = build(written, answer);
  popup.id = 'wlib-popup';
  // NOTE: In the box that scrolls (dom.js), so the popup scrolls with its word.
  const box = scroller();
  const page = box === document.scrollingElement;
  (page ? document.documentElement : box).append(popup);

  // NOTE: The status line bounds the visible reading area: under it, and above its row of keys on a touch screen,
  //  where that row is at the bottom.
  const root = document.documentElement;
  const gap = 4;
  const top = readingTop() + gap;
  const bottom = ('touch' in root.dataset ? document.querySelector('#status .keys').getBoundingClientRect().top
    : root.clientHeight) - gap;
  const belowWord = Math.max(rect.bottom + gap, top);
  const aboveWord = Math.min(rect.top - gap, bottom);
  const belowSpace = bottom - belowWord;
  const aboveSpace = aboveWord - top;
  const height = popup.offsetHeight;
  const below = height <= belowSpace || (height > aboveSpace && belowSpace >= aboveSpace);
  const space = below ? belowSpace : aboveSpace;
  if (height > space) popup.style.maxHeight = `${Math.max(0, space)}px`;
  const width = popup.offsetWidth;
  const left = Math.max(gap, Math.min(rect.left, root.clientWidth - width - gap));
  const origin = page ? { left: -window.scrollX, top: -window.scrollY } : box.getBoundingClientRect();
  const scrolled = page ? { left: 0, top: 0 } : { left: box.scrollLeft, top: box.scrollTop };
  popup.style.left = `${left - origin.left + scrolled.left}px`;
  popup.style.top = `${(below ? belowWord : aboveWord - popup.offsetHeight) - origin.top + scrolled.top}px`;
}
