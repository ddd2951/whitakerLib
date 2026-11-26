// INFO: Fuzzy title or passage search over recent texts in the library overlay (Your library).
import { $, element } from './dom.js';
import { plain } from './popup.js';

function score(text, query) {
  const value = plain(text).toLowerCase();
  let total = 0;
  for (const word of query) {
    const start = value.indexOf(word);
    if (start >= 0) {
      total += start / (start + 1);
      continue;
    }
    let at = 0;
    for (const letter of word) {
      at = value.indexOf(letter, at) + 1;
      if (!at) return Infinity;
    }
    total += 1 + at / (at + 1);
  }
  return total;
}

export function initFinder({ entries, activeId, select, remove }) {
  const search = $('library-search');
  const list = $('library-results');
  let recent = [];
  let matches = [];
  let selected = 0;
  let listing = 0;

  function mark(index, scroll = false) {
    selected = Math.max(0, Math.min(index, matches.length - 1));
    for (const [i, row] of [...list.children].entries()) row.classList.toggle('chosen', i === selected);
    if (scroll) list.children[selected]?.scrollIntoView({ block: 'nearest' });
  }

  function render(preferred = null) {
    const query = plain(search.value.trim()).toLowerCase().split(/\s+/).filter(Boolean);
    matches = recent.map(entry => [entry, score(entry.label, query)])
      .filter(([, rank]) => rank !== Infinity).sort((a, b) => a[1] - b[1]).map(([entry]) => entry);
    list.replaceChildren();
    $('library-empty').hidden = matches.length !== 0;
    for (const entry of matches) {
      const row = element('li');
      const open = element('button', 'library-open', entry.label);
      open.type = 'button';
      if (entry.id === activeId()) open.setAttribute('aria-current', 'true');
      open.addEventListener('click', () => select(entry.id));
      row.append(open);
      if (entry.kind !== 'sample') {
        const drop = element('button', 'library-delete', 'Delete');
        drop.type = 'button';
        drop.setAttribute('aria-label', `Delete ${entry.label}`);
        drop.addEventListener('click', async () => {
          search.focus();
          await remove(entry.id);
          await refresh();
        });
        row.append(drop);
      }
      list.append(row);
    }
    mark(matches.findIndex(entry => entry.id === preferred), preferred !== null);
  }

  async function refresh(preferred = null) {
    const request = ++listing;
    const result = await entries();
    if (request !== listing) return;
    recent = result;
    render(search.value ? null : preferred);
  }

  search.addEventListener('input', () => render());
  search.addEventListener('keydown', e => {
    if (e.key === 'ArrowDown' || e.key === 'ArrowUp') {
      e.preventDefault();
      mark(selected + (e.key === 'ArrowDown' ? 1 : -1), true);
    } else if (e.key === 'Enter' && matches.length) {
      e.preventDefault();
      select(matches[selected].id);
    }
  });

  return function open() {
    search.value = '';
    render(activeId());
    search.focus();
    refresh(activeId());
  };
}
