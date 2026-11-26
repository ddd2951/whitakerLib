// INFO: Guesses the main verbs, clauses and compound verbs from the inspector's flags for each word.
import { capitalizedInside, endsAfter } from './text.js';

const FINITE = 1, QUE = 32, ABLATIVE = 64;
export const INFINITIVE = 4, UNKNOWN = 8, PARTICIPLE = 16, NOT_VERB = 128, ONLY_NOMINATIVE = 256, NOMINATIVE = 512;

// NOTE: A verb with a person, and nothing else: many common words have a rare verb reading as well (eo 'him' and
//  'I go', duce 'leader' and 'lead!'), and an infinitive reading usually wins (sperare).
const verb = m => !!(m & FINITE) && !(m & (NOT_VERB | INFINITIVE));

// NOTE: Words that open a clause whose verb is not the main one; structureOf says when cum, quam and a relative
//  don't.
const OPENERS = new Set(`cum ut uti ne si nisi quod quia quoniam postquam posteaquam ubi dum donec quamquam quamvis
  etsi etiamsi tametsi antequam priusquam simul simulac simulatque quasi tamquam velut sicut licet qui quae quod cuius
  cui quem quam quo qua quorum quarum quibus quos quas qualis quale qualem quales qualium qualibus quantus quanta
  quantum quantam quanto quot quotiens unde quando cur quare quomodo utrum num`.split(/\s+/));
const COORDINATORS = new Set('et atque ac neque nec sed aut vel'.split(' '));
// NOTE: Words that start the main clause after a side clause (etsi … ceciderat, tamen …: 'although …, nevertheless'):
//  after one, 'sed' or 'et' no longer carries the side clause on.
const MAIN_STARTS = new Set('tamen attamen nihilominus itaque igitur ergo ideo'.split(' '));
const THAN_AFTER = new Set('minus magis potius plus prius tam amplius'.split(' '));
const COMPARATIVE = /(ius|ior|iores|iorem|iori|iorum|ioribus)$/;
export const RELATIVES = new Set('qui quae quod cuius cui quem quam quo qua quorum quarum quibus quos quas'.split(' '));
// NOTE: A preposition that a sentence may open with before a linking relative (in quo, inter quos).
const PREPOSITIONS = new Set('a ab ad cum de e ex in inter ob per post pro propter sub'.split(' '));
const SUM = new Set(`est sunt erat erant fuit fuerunt esset essent sit sint fuerat fuerant fuisset fuissent fore esse
  fuisse`.split(/\s+/));
const BOUNDARY = /[.;:!?]/;

let marks = [];
let verbs = [];
let structure = { main: [], clauses: [], compound: new Map() };

// NOTE: A clause opener waits for the next verb with a person: that verb is its clause's (the innermost clause open
//  takes it). A verb nothing waits for is a main verb. After a subordinate verb, 'et' or '-que' carries the same
//  clause on (cum … floreret eaque esset: one clause, two verbs). Each stretch between . ; : ! ? is judged on its own;
//  a clause still open at its end found no verb. Clauses come in the order they open: { opener, verbs, depth }.
//  Depth 0 hangs on the main verb; a clause that opens inside another, or right after a side clause closed and before
//  the main verb, hangs on that one (cum … esset aetate, ut … possent: 'of such an age that …'). A guess from word
//  order, as the rest.
//
// NOTE: When a word opens a clause, and when it doesn't (from Nepos' Miltiades):
//  - a relative that opens a sentence (or follows its first preposition) links to the sentence before ('and because
//    of this': quo factum est, quibus rebus fiebat, inter quos): it opens no clause;
//  - cum is 'when' if a verb follows before the next comma with no other opener first, else 'with' when an ablative
//    follows it (cum virtute militum devicisset: 'when'; cum delecta manu … cum accessisset: 'with', then 'when');
//  - quam is 'than' after a comparison anywhere earlier in the stretch (minus … quam);
//  - et, -que carry on a cum or ut clause, never a relative one (quo tendebat, pervenitque: pervenit is main);
//  - a participle next to a form of sum is one verb with it (ausi non sunt, accusatus est): compound maps the form
//    of sum to its participle.
function structureOf(text, words) {
  const main = [];
  const clauses = [];
  const compound = new Map();
  let open = [];
  let lastClosed = null;
  let start = 0;
  const lower = k => words[k].spelling.toLowerCase();
  const stretchEnds = k => k + 1 >= words.length || endsAfter(text, words, k, BOUNDARY);
  words.forEach((o, i) => {
    if (i > 0 && endsAfter(text, words, i - 1, BOUNDARY)) {
      open = [];
      lastClosed = null;
      start = i;
    }
    const w = lower(i);
    const m = marks[i];
    const linking = RELATIVES.has(w) && (i === start || i === start + 1 && PREPOSITIONS.has(lower(start)));
    let verbAhead = false;
    if (w === 'cum')
      for (let k = i + 1; k < words.length; ++k) {
        if (OPENERS.has(lower(k))) break;
        if (verbs[k]) {
          verbAhead = true;
          break;
        }
        if (stretchEnds(k) || text.slice(words[k].end, words[k + 1].start).includes(',')) break;
      }
    const preposition = w === 'cum' && !verbAhead && i + 1 < words.length && marks[i + 1] & ABLATIVE;
    const than = w === 'quam' && [...Array(i - start).keys()].some(k =>
      THAN_AFTER.has(lower(start + k)) || COMPARATIVE.test(lower(start + k)));
    if (OPENERS.has(w) && !preposition && !than && !linking) {
      const depth = open.length ? open.at(-1).depth + 1 : lastClosed ? lastClosed.depth + 1 : 0;
      const clause = { opener: i, verbs: [], depth };
      clauses.push(clause);
      open.push(clause);
      return;
    }
    if (MAIN_STARTS.has(w) && !open.length) lastClosed = null;
    if ((COORDINATORS.has(w) || m & QUE) && !open.length && lastClosed && !RELATIVES.has(lower(lastClosed.opener)))
      open.push(lastClosed);
    if (!verbs[i]) return;
    if (open.length) {
      lastClosed = open.pop();
      lastClosed.verbs.push(i);
    } else {
      main.push(i);
      lastClosed = null;
    }
  });
  for (const j of [...main, ...clauses.flatMap(c => c.verbs)]) {
    if (!SUM.has(lower(j))) continue;
    const apart = k => [...Array(Math.abs(j - k)).keys()].some(n =>
      endsAfter(text, words, Math.min(j, k) + n, BOUNDARY));
    const near = [j - 1, j - 2, j + 1, j - 3].find(k =>
      k >= 0 && k < words.length && marks[k] & PARTICIPLE && !apart(k));
    if (near !== undefined) compound.set(j, near);
  }
  return { main, clauses, compound };
}

// INFO: Whether word i is a verb with a person.
export const isVerb = i => !!verbs[i];
export const flagsOf = i => marks[i] ?? 0;
// INFO: Whether word i goes with a sentence's verbs: a verb with a person, a participle, or an infinitive and nothing
//  else (sperare, mittere; not sui, which reads as suo 'sew' too).
export const isVerbal = i => isVerb(i) || !!(flagsOf(i) & PARTICIPLE) ||
  !!(flagsOf(i) & INFINITIVE) && !(flagsOf(i) & NOT_VERB);
export const structureNow = () => structure;

export function load(text, words, flags) {
  marks = words.map((_, i) => parseInt(flags.substr(4 * i, 4), 16));
  // NOTE: Not a capitalised word inside a sentence: Cares (a people) reads as careo, 'lack'.
  verbs = marks.map((m, i) => verb(m) && !capitalizedInside(text, words, i));
  structure = structureOf(text, words);
}
