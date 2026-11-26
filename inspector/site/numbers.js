// INFO: The text's numbers above it: the proof line, then one folded line a block, each saying its main fact and
//  opening on a click: readings per word, the parts of speech, the commonest spellings, those with no reading.
import { $, count, element } from './dom.js';

// INFO: btop's bars: whole blocks, then an eighth block for the rest.
function bar(value, max, cells = 30) {
  const n = max ? (value / max) * cells : 0;
  const eighths = Math.round(n * 8);
  return '█'.repeat(Math.floor(eighths / 8)) + ['', '▏', '▎', '▍', '▌', '▋', '▊', '▉'][eighths % 8];
}

function folded(title, gist) {
  const block = element('details', 'block');
  const summary = element('summary');
  summary.append(element('span', 'head', title), element('span', 'gist', gist));
  block.append(summary);
  return block;
}

function bars(title, gist, rows) {
  const block = folded(title, gist);
  const max = Math.max(...rows.map(([, v]) => v));
  for (const [label, value] of rows) {
    const row = element('div', 'bar-row');
    row.append(element('span', '', label), element('span', 'bar', bar(value, max)),
               element('span', 'num', count(value)));
    block.append(row);
  }
  return block;
}

// INFO: Spellings by how often they occur, the commonest first; a click goes to the next place one occurs (page.js).
//  The list is built the first time it is opened: folded, it costs nothing.
function spellingList(title, counts) {
  const sorted = [...counts].sort((a, b) => b[1] - a[1]);
  const gist = sorted.length ? `${sorted.slice(0, 5).map(([w]) => w).join(', ')} …` : 'none';
  const block = folded(`${title} (${count(counts.size)})`, gist);
  block.addEventListener('toggle', () => {
    if (!block.open || block.querySelector('.cols')) return;
    const list = element('div', 'cols');
    for (const [spelling, n] of sorted.slice(0, 40)) {
      const item = element('span', 'find');
      item.dataset.spelling = spelling;
      item.append(spelling, element('span', 'n', ` ${count(n)}`));
      list.append(item);
    }
    block.append(list);
  });
  return block;
}

// INFO: words and known as text.js holds them; counts as the library's read_text gives them. The timing line is left
//  empty, for page.js to fill once the text is drawn.
export function numbers(words, known, counts) {
  // NOTE: Spellings counted once, case folded, for the proof line and both lists.
  const all = new Map();
  const unread = new Map();
  let read = 0;
  for (let i = 0; i < words.length; ++i) {
    const w = words[i].toLowerCase();
    all.set(w, (all.get(w) ?? 0) + 1);
    if (known.charCodeAt(i) === 49) ++read;
    else unread.set(w, (unread.get(w) ?? 0) + 1);
  }
  const out = $('numbers');
  out.replaceChildren();
  out.append(element('p', 'proof', `${count(words.length)} words, ${count(all.size)} spellings: ` +
    `${(100 * read / words.length).toFixed(1)}% have a reading.`));
  out.append(element('p', 'timing'));
  const perWord = new Map();
  for (const [n, c] of counts.readings) {
    const key = n >= 10 ? '10 or more' : String(n);
    perWord.set(key, (perWord.get(key) ?? 0) + c);
  }
  const rows = [...perWord].map(([n, c]) => [n === '1' ? '1 reading' : `${n} readings`, c]);
  const most = rows.reduce((a, b) => (b[1] > a[1] ? b : a), ['', 0]);
  out.append(bars('Readings per word', `most have ${most[0]} (${count(most[1])} words)` +
    (perWord.has('10 or more') ? `, ${count(perWord.get('10 or more'))} have 10 or more` : ''), rows));
  const commonest = counts.parts.slice(0, 3).map(([p, c]) => `${p} ${count(c)}`).join(' · ');
  out.append(bars('Possible parts of speech', `${commonest} …`, counts.parts));
  out.lastChild.append(element('p', 'dim', 'A word counts once for each part of speech it could be. Cano, for ' +
    'example, counts as a noun, an adjective and a verb.'));
  out.append(spellingList('Most common spellings', all));
  out.append(spellingList('Unrecognised spellings', unread));
}

export function timing(text) {
  document.querySelector('#numbers .timing').textContent = text;
}
