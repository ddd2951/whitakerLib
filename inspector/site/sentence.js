// INFO: The Sentence view: the selected word's sentence taken apart, as a reader learning Latin would do it by hand.
//  The reading the selected word most likely has there and why, the sentence in pieces with what each holds, its
//  main verb, its likely subject, and the words the library doesn't know. Everything here is a guess from the words
//  alone (highlights.js, structureOf): a start for reading, not an answer. Words are buttons: a tap selects that
//  word.
//
// NOTE: The page answers the view's buttons by their data: data-word selects a word, data-action="ask-sentence"
//  copies the sentence for Google help (app.js).

import { placeOf, rankOf, recordsOf, sentenceRange } from './ask.js';
import { capitalizedInside } from './text.js';
import { flagsOf, INFINITIVE, isVerb, NOMINATIVE, NOT_VERB, ONLY_NOMINATIVE, PARTICIPLE, RELATIVES, structureNow,
  UNKNOWN } from './highlights.js';

// What the word that opens a clause usually means; a relative pronoun (qui, quae, quod, cuius …) is 'who, which'.
const GLOSSES = new Map(Object.entries({
  cum: 'when, since, although', ut: 'that, so that, as, when', uti: 'that, so that, as', ne: 'that … not, so as not to',
  si: 'if', nisi: 'unless, if not', quia: 'because', quoniam: 'since, because',
  postquam: 'after', posteaquam: 'after', ubi: 'when, where', dum: 'while, until', donec: 'until, while',
  quamquam: 'although', quamvis: 'although', etsi: 'although, even if', etiamsi: 'even if', tametsi: 'although',
  licet: 'although', antequam: 'before', priusquam: 'before', simul: 'as soon as', simulac: 'as soon as',
  simulatque: 'as soon as', quasi: 'as if', tamquam: 'as if, just as', velut: 'just as', sicut: 'just as',
  quantus: 'as great as', quanta: 'as great as', quantum: 'as much as', quantam: 'as great as', quanto: 'by as much as',
  quot: 'as many as', quotiens: 'as often as', unde: 'from where', quando: 'when', cur: 'why', quare: 'why',
  quomodo: 'how', utrum: 'whether', num: 'whether',
}));
const QUALIS = new Set('qualis quale qualem quales qualium qualibus'.split(' '));

// NOTE: Verbs whose subject is a clause (accidit ut …: 'it happened that …'), besides those WORDS marks IMPERS.
const IMPERSONAL = new Set(`accidit accidebat acciderat fit fiebat factum contigit evenit evenerat placet placuit licet
  licuit oportet oportuit constat constabat apparet convenit`.split(/\s+/));
const CLAUSE_SUBJECT = new Set(['ut', 'uti', 'ne', 'quod']);
// The subject a verb's person stands for, when no word in the sentence is it.
const PRONOUNS = new Map([['1st person singular', 'I'], ['2nd person singular', 'you'], ['3rd person singular',
  'he/she/it'], ['1st person plural', 'we'], ['2nd person plural', 'you (all)'], ['3rd person plural', 'they']]);
// NOTE: A name's case is not known, but these endings are almost never a nominative (Cimonis, Lemnum, Atheniensium).
const OBLIQUE = /(is|i|o|um|em|am|ae|orum|arum|ibus|os|as)$/;

function glossOf(word) {
  const w = word.toLowerCase();
  if (QUALIS.has(w)) return 'of such a kind as';
  if (w === 'quod') return 'which, that, because';
  if (w === 'quam') return 'whom, which; than';
  return RELATIVES.has(w) ? 'who, which' : GLOSSES.get(w) ?? '';
}

function el(tag, cls, text) {
  const e = document.createElement(tag);
  if (cls) e.className = cls;
  if (text !== undefined) e.textContent = text;
  return e;
}

function wordButton(words, j, cls = '') {
  const b = el('button', `s-word ${cls}`.trim(), words[j].text);
  b.dataset.word = j;
  return b;
}

// NOTE: A verb's dictionary words, an impersonal one first, then the most common (WORDS' frequency, A to F): WORDS'
//  own order put accidit's 'cut' before its 'happen'. The first is shown with its forms; the others by their meaning,
//  shortened.
function verbReadings(html) {
  const records = recordsOf(html).sort((a, b) => rankOf(a) - rankOf(b)).sort((a, b) => b.impersonal - a.impersonal);
  if (!records.length) return null;
  const forms = [...new Set(records[0].forms.map(f => f.words))];
  const short = m => m.length > 40 ? `${m.slice(0, 40)}…` : m;
  return { meaning: records[0].meaning, form: forms.join(' or '), others: records.slice(1).map(r => short(r.meaning)),
           impersonal: records.some(r => r.impersonal) };
}

// The case a preposition takes: its next word's reading is that one (ab urbe: ablative; in takes either).
const TAKES = new Map([
  ...'a ab abs cum de e ex sine pro prae'.split(' ').map(p => [p, /^ablative/]),
  ...'ad ante apud circum contra erga inter ob per post praeter propter trans'.split(' ').map(p => [p, /^accusative/]),
  ...'in sub super'.split(' ').map(p => [p, /^(ablative|accusative)/])]);

// NOTE: The reading word i most likely has in its sentence, and why: from its part in it (analyse) or the word before
//  it, else the commonest reading, and not a verb's if the sentence doesn't take it for one (loca: locus, not loco).
//  The dictionary words are tried most common first (an impersonal verb's impersonal one first); the first with a
//  form that fits wins.
export function expectedOf({ words, i, a, readingsHtmlOf }) {
  const records = recordsOf(readingsHtmlOf(i)).sort((x, y) => rankOf(x) - rankOf(y))
    .sort((x, y) => (a.impersonal === i) * (y.impersonal - x.impersonal));
  const pick = (fits, why) => {
    for (const record of records) {
      // NOTE: A preposition's form is the case it takes (ablative); said as what it is.
      const forms = record.forms.filter(f => fits(f.words)).map(f =>
        record.meta.startsWith('preposition') ? { ...f, words: `preposition, with the ${f.words}` } : f);
      if (forms.length) return { record, forms, why };
    }
    return null;
  };
  const before = i > 0 ? words[i - 1].text.toLowerCase() : '';
  const topHas = form => !!records[0]?.forms.some(f => f.words.includes(form));
  // The number of verb k (singular or plural), from its first form with a person; null if it has none.
  const numberOf = k => k === undefined ? null : recordsOf(readingsHtmlOf(k)).flatMap(r => r.forms)
    .map(f => f.words.match(/person (singular|plural)/)?.[1]).find(Boolean) ?? null;
  const clause = a.here.find(c => c.verbs.includes(i));
  const opener = a.here.find(c => c.opener === i);
  const pairedWith = [...a.compound].find(([, k]) => k === i)?.[0];
  return (a.mains.includes(i) && pick(w => w.includes('person'), 'the main verb')) ||
    (clause && pick(w => w.includes('person'), `the verb of the ${words[clause.opener].text} clause`)) ||
    (pairedWith !== undefined && pick(w => w.startsWith('nominative') && w.includes('participle'),
                                      `with ${words[pairedWith].text}: one verb, a perfect`)) ||
    // NOTE: A relative that opens a clause takes the number of its clause's verb (qui … liberarat: one man).
    (opener && RELATIVES.has(words[i].text.toLowerCase()) && numberOf(opener.verbs[0]) &&
     pick(w => w.startsWith(`nominative ${numberOf(opener.verbs[0])}`),
          `opens a clause: who, which; ${numberOf(opener.verbs[0])}, as its verb ${words[opener.verbs[0]].text}`)) ||
    (opener && pick(() => true, ['opens a clause', glossOf(words[i].text)].filter(Boolean).join(': '))) ||
    // NOTE: cum that opens no clause here is the preposition (cum delecta manu: 'with'); its form is the case it takes.
    // TODO: cum … tum later in the stretch is 'both … and' (cum summa humanitas tum mira communitas), not 'with'.
    (words[i].text.toLowerCase() === 'cum' &&
     pick(w => w === 'ablative', 'with: the preposition, opening no clause')) ||
    (a.subjects.some(([j]) => j === i) && pick(w => w.startsWith('nominative'), 'the subject, or a word with it')) ||
    (TAKES.has(before) && pick(w => TAKES.get(before).test(w), `after ${before}, which takes this case`)) ||
    // NOTE: An infinitive or a participle only when the word's commonest dictionary word is one: esse is sum's
    //  infinitive, not edo's participle; libertate is libertas, not a participle of libero.
    (topHas('infinitive') && pick(w => w.includes('infinitive'), 'an infinitive')) ||
    (records[0]?.meta.startsWith('verb') && topHas('participle') &&
     pick(w => w.includes('participle'), 'a participle (its piece says more)')) ||
    (!isVerb(i) && pick(w => !w.includes('person'), 'no clue from the sentence yet: the commonest reading')) ||
    pick(() => true, 'no clue from the sentence yet: the commonest reading');
}

function section(title, ...content) {
  const row = el('div', 's-row');
  row.append(el('div', 's-title', title));
  const body = el('div', 's-body');
  body.append(...content);
  row.append(body);
  return row;
}

// A verb's meaning, short: the most common dictionary word's (the impersonal one's, for an impersonal verb), up to
//  its first ';'.
function meaningOf(html, impersonal = false) {
  const records = recordsOf(html).sort((a, b) => rankOf(a) - rankOf(b));
  const record = (impersonal ? records.find(r => r.impersonal) : undefined) ??
                 records.find(r => r.meta.startsWith('verb')) ?? records[0];
  const m = record?.meaning.split(';')[0].trim() ?? '';
  return m.length > 32 ? `${m.slice(0, 32)}…` : m;
}

// NOTE: The sentence in pieces, cut at its commas: the editor's, but in Nepos they fall where a reader pauses. Each
//  piece says what it holds, from the endings, not the commas: the main verb, the clauses that open, run on or end in
//  it (indented by how deep they sit), its infinitives, a future participle standing for an infinitive with esse left
//  out (talem eum futurum: acc. + inf.), or nothing of its own.
function piecesOf({ text, words, from, to, mains, compound, here, classOf, readingsHtmlOf, impersonal }) {
  const pieces = [];
  for (let start = from, j = from; j <= to; ++j)
    if (j === to || text.slice(words[j].end, words[j + 1].start).includes(',')) {
      pieces.push([start, j]);
      start = j + 1;
    }
  const quoted = j => ` "${meaningOf(readingsHtmlOf(j), j === impersonal)}"`;
  const rows = pieces.map(([a, b]) => {
    const inside = j => j >= a && j <= b;
    const labels = [];
    for (const j of mains.filter(inside))
      labels.push([0, compound.has(j)
        ? ['★ main: ', wordButton(words, compound.get(j), 's-main'), ' … ', wordButton(words, j, 's-main'),
           quoted(compound.get(j))]
        : ['★ main: ', wordButton(words, j, 's-main'), quoted(j)]]);
    for (const c of here) {
      const end = c.verbs.at(-1) ?? to;
      const verbsHere = c.verbs.filter(inside);
      const opens = inside(c.opener);
      if (!opens && !verbsHere.length && !(c.opener < a && end > b)) continue;
      const status = opens ? (end > b ? ', starts' : '') : verbsHere.length && end <= b ? ', ends' : ', runs on';
      const parts = [wordButton(words, c.opener, 's-opener'), ` clause${status}`];
      for (const v of verbsHere) parts.push(' · ', wordButton(words, v, 's-verb'), quoted(v));
      labels.push([c.depth, parts]);
    }
    const finite = [...Array(b - a + 1).keys()].some(k => isVerb(a + k));
    for (let j = a; j <= b; ++j) {
      if (flagsOf(j) & INFINITIVE && !(flagsOf(j) & NOT_VERB))
        labels.push([0, ['infinitive · ', wordButton(words, j), quoted(j)]]);
      const records = recordsOf(readingsHtmlOf(j));
      const future = !finite && records.some(r => r.forms.some(f =>
        f.words.includes('future active participle') && f.words.startsWith('accusative')));
      if (future) labels.push([0, ['acc. + inf. · ', wordButton(words, j), ' [esse]', quoted(j)]]);
      // NOTE: A participle, when the word's commonest dictionary word is a verb (not dicto, which is dictum too).
      const top = [...records].sort((x, y) => rankOf(x) - rankOf(y))[0];
      const inCompound = [...compound.values()].includes(j);
      const participle = !future && !inCompound && flagsOf(j) & PARTICIPLE && top?.meta.startsWith('verb') &&
                         top.forms.some(f => f.words.includes('participle'));
      // NOTE: A common noun of the same spelling (frequency A to C) is said too: only the context tells dicto 'said'
      //  from dictum 'word', and WORDS' frequency puts the verb first for both dicto and ausi.
      if (participle) {
        const noun = records.find(r => r.meta.startsWith('noun') && 'ABC'.includes(r.frequency || 'X'));
        const nounMeaning = noun?.meaning.split(';')[0].trim();
        labels.push([0, ['participle · ', wordButton(words, j), quoted(j),
                         ...(noun ? [` · or a noun: "${nounMeaning}"`] : [])]]);
      }
    }
    const row = el('div', 's-piece');
    const piece = el('div', 's-piece-text');
    for (let j = a; j <= b; ++j) piece.append(j > a ? ' ' : '', wordButton(words, j, classOf(j)));
    const notes = el('div', 's-piece-notes');
    for (const [depth, parts] of labels) {
      const line = el('div', 's-line');
      line.style.marginLeft = `${depth * 1.2}em`;
      line.append(...parts);
      notes.append(line);
    }
    row.append(piece, notes);
    return row;
  });
  const all = el('div', 's-pieces');
  all.append(...rows);
  return all;
}

// NOTE: The sentence holding word i, taken apart: what the view shows, as data, apart from drawing it. Words are
//  their places in the text.
export function analyse({ text, words, i, readingsHtmlOf }) {
  const [from, to] = sentenceRange(text, words, i);
  const { main, clauses, compound } = structureNow();
  const mains = main.filter(j => j >= from && j <= to);
  // A main verb as its words: a form of sum with its participle (ausi … sunt), or the verb alone.
  const mainWords = mains.map(j => compound.has(j) ? [compound.get(j), j].sort((x, y) => x - y) : [j]);
  const here = clauses.filter(c => c.opener >= from && c.opener <= to);
  const inClause = j => here.some(c => j >= c.opener && j <= (c.verbs.at(-1) ?? to));
  const subjects = [];
  const unknown = [];
  for (let j = from; j <= to; ++j) {
    const m = flagsOf(j);
    if (m & UNKNOWN) unknown.push(j);
    if (inClause(j) || m & PARTICIPLE) continue;
    if (m & ONLY_NOMINATIVE) subjects.push([j, '']);
    // NOTE: A name: unknown and capitalised, or capitalised where only a name would be (Cares, read as careo).
    else if ((m & UNKNOWN && /^\p{Lu}/u.test(words[j].text) || capitalizedInside(text, words, j)) &&
             !OBLIQUE.test(words[j].text)) subjects.push([j, '?']);
  }
  // NOTE: A word that can be a nominative, next to one found, goes with it (filius, Atheniensis: 'son …, an
  //  Athenian'); repeated, so a run of them joins.
  for (let grew = true; grew;) {
    grew = false;
    for (let j = from; j <= to; ++j)
      if (!inClause(j) && flagsOf(j) & NOMINATIVE && !(flagsOf(j) & PARTICIPLE) &&
          !subjects.some(([k]) => k === j) && subjects.some(([k]) => Math.abs(k - j) === 1)) {
        subjects.push([j, '']);
        grew = true;
      }
  }
  // NOTE: A subject agrees in number with a main verb: loca (neuter plural) is not the subject of communiit. Names
  //  ('?') stay: their number is not known.
  const numbersIn = (j, pattern) => new Set(recordsOf(readingsHtmlOf(j)).flatMap(r =>
    r.forms.flatMap(f => [...f.words.matchAll(pattern)].map(m => m[1]))));
  const verbNumbers = new Set(mains.flatMap(j => [...numbersIn(j, /person (singular|plural)/g)]));
  const agrees = ([j, mark]) => mark === '?' || !verbNumbers.size ||
    [...numbersIn(j, /^nominative (singular|plural)/g)].some(n => verbNumbers.has(n));
  subjects.splice(0, subjects.length, ...subjects.filter(agrees));
  subjects.sort((a, b) => a[0] - b[0]);
  const verbs = new Map(mains.map(j => [j, verbReadings(readingsHtmlOf(j))]));
  // The clause that is the subject of an impersonal main verb (accidit ut …), if there is one.
  const clauseSubject = mains.flatMap(j => {
    if (!IMPERSONAL.has(words[j].text.toLowerCase()) && !verbs.get(j)?.impersonal) return [];
    const c = here.find(c => c.opener > j && CLAUSE_SUBJECT.has(words[c.opener].text.toLowerCase()));
    return c ? [[j, c]] : [];
  })[0];
  const persons = new Set(mains.flatMap(j => [...numbersIn(j, /(\d\w\w person (?:singular|plural))/g)]));
  const implied = !clauseSubject && !subjects.length && persons.size === 1 ? [...persons][0] : null;
  return { from, to, mains, mainWords, compound, here, subjects, unknown, verbs, clauseSubject, implied };
}

export function render(container, { text, words, i, readingsHtmlOf }) {
  const { from, to, mains, mainWords, compound, here, subjects, unknown, verbs, clauseSubject, implied } =
    analyse({ text, words, i, readingsHtmlOf });
  const clauseVerbs = new Set(here.flatMap(c => c.verbs));
  const openers = new Set(here.map(c => c.opener));
  const subjectSet = new Set(subjects.map(([j]) => j));

  const out = [];
  out.push(el('p', 's-warning', 'Experimental: this tests how much of a sentence\'s structure its words alone show. ' +
                                 'Its reading may be very wrong.'));
  out.push(el('div', 's-place', placeOf(text, words[from].start)));

  const expected = expectedOf({ words, i, a: { mains, compound, here, subjects, impersonal: clauseSubject?.[0] },
                                readingsHtmlOf });
  if (expected) {
    const { record, forms, why } = expected;
    const line = el('div', 's-line');
    line.append(wordButton(words, i, 's-here'), el('span', 's-meaning', ` "${record.meaning.split(';')[0].trim()}"`));
    // NOTE: An indeclinable's form is a WORDS code (ADV POS); it says nothing a reader needs.
    for (const f of forms.filter(f => !/^[A-Z0-9 ]+$/.test(f.words)))
      line.append(el('div', 's-form', `${f.split}: ${f.words}`));
    line.append(el('div', 's-note', `why: ${why}`));
    out.push(section('This word here', line));
  }

  const classOf = j => [mains.includes(j) ? 's-main' : clauseVerbs.has(j) ? 's-verb' : '',
                         openers.has(j) ? 's-opener' : '', subjectSet.has(j) ? 's-subject' : '',
                         j === i ? 's-here' : ''].filter(Boolean).join(' ');
  out.push(piecesOf({ text, words, from, to, mains, compound, here, classOf, readingsHtmlOf,
                      impersonal: clauseSubject?.[0] }));

  const mainLines = mains.map((j, n) => {
    const line = el('div', 's-line');
    // NOTE: A participle with a form of sum is one verb: its meaning is the participle's (ausi … sunt: audeo 'dare').
    if (compound.has(j)) {
      const k = compound.get(j);
      const r = verbReadings(readingsHtmlOf(k));
      mainWords[n].forEach((w, m) => line.append(m ? ' ' : '', wordButton(words, w, 's-main')));
      if (r) line.append(el('span', 's-meaning', ` "${r.meaning}"`));
      line.append(el('span', 's-form', ' a perfect'));
      return line;
    }
    const r = verbs.get(j);
    line.append(wordButton(words, j, 's-main'));
    if (!r) return line;
    line.append(el('span', 's-meaning', ` "${r.meaning}"`), el('span', 's-form', ` ${r.form}`));
    if (r.others.length) line.append(el('div', 's-note', `also: ${r.others.map(m => `"${m}"`).join(' · ')}`));
    return line;
  });
  out.push(section('Main verb', ...(mainLines.length ? mainLines : [el('span', 's-none', 'none found')])));

  const subjectLine = el('div', 's-line');
  subjects.forEach(([j, mark], k) => {
    if (k) subjectLine.append(' · ');
    subjectLine.append(wordButton(words, j, 's-subject'), mark);
  });
  if (clauseSubject) {
    const [, c] = clauseSubject;
    const line = el('div', 's-line');
    line.append('the clause ', wordButton(words, c.opener, 's-opener'), ' … ',
                ...c.verbs.flatMap((v, k) => [k ? ', ' : '', wordButton(words, v, 's-verb')]));
    out.push(section('Subject', line));
  } else {
    const none = PRONOUNS.has(implied) ? `implied: "${PRONOUNS.get(implied)}"` : 'none found';
    out.push(section('Subject', subjects.length ? subjectLine : el('span', 's-none', none)));
  }

  if (unknown.length) {
    const line = el('div', 's-line');
    unknown.forEach((j, k) => line.append(k ? ', ' : '', wordButton(words, j)));
    out.push(section('Not in whitakerLib', line));
  }

  const ask = el('button', 's-ask', 'Copy sentence for Google help');
  ask.dataset.action = 'ask-sentence';
  out.push(ask);
  container.replaceChildren(...out);
}
