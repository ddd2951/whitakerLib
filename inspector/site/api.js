// INFO: The api window: one word through the library's API, as a programmer would call it in C++, C or Python, with
//  what each call returns (library.api, ../page.cpp). Which functions show and in which binding are picked here and
//  kept in this browser. The word is the one last opened in the text, or one typed in the box.
import { $, element } from './dom.js';
import * as popup from './popup.js';

const BINDINGS = ['C++', 'C', 'Python'];
// NOTE: The lines before the calls, as the README's examples write them.
const SETUP = [
  (w, status) => [`whitaker::Result result;`, `whitaker::analyze(result, "${w}");  // whitaker::Status::${status}`,
    'const whitaker::WordId word{0};'],
  (w, status) => ['WhitakerResult* result = whitaker_result_new();',
    `whitaker_analyze(result, "${w}", ${w.length});  // WHITAKER_STATUS_${status.replace(/[A-Z]/g, c => `_${c}`)
      .toUpperCase()}`, 'const size_t word = 0;'],
  (w, status) => ['result = whitaker.Result()', `result.analyze('${w}')  # Status.${status}`, 'word = 0'],
];
const FUNCTIONS = ['spelling', 'trimmed', 'readingCount', 'detail', 'stems', 'addonSpelling', 'addonMeaning'];

let library = null;
let picked = { binding: 0, functions: ['readingCount', 'detail'] };
try {
  picked = { ...picked, ...JSON.parse(localStorage.getItem('api')) };
} catch {
  // NOTE: No storage: the defaults
}

function keep() {
  try {
    localStorage.setItem('api', JSON.stringify(picked));
  } catch {
    // NOTE: Not kept: it holds until the page is closed
  }
}

function choices() {
  const out = $('api-choices');
  out.replaceChildren();
  const line = element('p');
  BINDINGS.forEach((name, b) => {
    const button = element('button', b === picked.binding ? 'on' : '', name);
    button.type = 'button';
    button.addEventListener('click', () => { picked.binding = b; keep(); choices(); show(); });
    line.append(button, ' ');
  });
  out.append(line);
  const functions = element('p');
  for (const f of FUNCTIONS) {
    const label = element('label');
    const box = element('input');
    box.type = 'checkbox';
    box.checked = picked.functions.includes(f);
    box.addEventListener('change', () => {
      picked.functions = FUNCTIONS.filter(g => (g === f ? box.checked : picked.functions.includes(g)));
      keep();
      show();
    });
    label.append(box, ` ${f} `);
    functions.append(label, ' ');
  }
  out.append(functions);
}

export function init(lib) {
  library = lib;
  choices();
  $('api-word').addEventListener('change', show);
}

export function setWord(written) {
  $('api-word').value = popup.plain(written);
}

export function show() {
  const view = $('api-view');
  const word = popup.plain($('api-word').value.trim());
  if (!library) return;
  if (!word) {
    view.textContent = 'Open a word in the text or type one, then its calls show here.';
    return;
  }
  const { status, calls } = library.api(word, picked.binding);
  const comment = picked.binding === 2 ? '  # ' : '  // ';
  view.replaceChildren(element('pre', 'call-line', SETUP[picked.binding](word, status).join('\n')));
  let first = true;
  for (const c of calls.filter(c => picked.functions.includes(c.function))) {
    const block = element('details', 'api-call');
    block.open = c.function !== 'detail' || first;
    first &&= c.function !== 'detail';
    block.append(element('summary', 'call-line', c.call + (c.note ? comment + c.note : '')));
    const table = element('table', 'rows');
    for (const [field, value] of c.rows) {
      const row = element('tr');
      // NOTE: A call that returns one value (no field) gets one cell, "→ value", so it stays on one line.
      if (field) row.append(element('td', 'path', field), element('td', 'value', value));
      else row.append(element('td', 'value', `→ ${value}`));
      table.append(row);
    }
    block.append(table);
    view.append(block);
  }
}
