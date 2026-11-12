// INFO: Puts the library's HTML for the selection on screen. Readable, API and Trace come 50 spellings at a time as
//  the view scrolls, since a whole book at once would stall the page; Statistics comes whole.
export const VIEWS = ['readable', 'api', 'trace', 'statistics'];

const STEP = 50;
let observer = null;

export function render(view, groups, library, container) {
  observer?.disconnect();
  observer = null;
  const v = VIEWS.indexOf(view);
  if (view === 'statistics') {
    container.innerHTML = library.view(v, 0, 0);
    return;
  }
  const sentinel = document.createElement('div');
  sentinel.className = 'more';
  container.replaceChildren(sentinel);
  let next = 0;
  const more = () => {
    sentinel.insertAdjacentHTML('beforebegin', library.view(v, next, STEP));
    next += STEP;
    if (next >= groups) {
      observer?.disconnect();
      sentinel.remove();
    } else {
      sentinel.textContent = `${groups - next} more …`;
    }
  };
  more();
  if (next < groups) {
    observer = new IntersectionObserver(entries => { if (entries.some(e => e.isIntersecting)) more(); },
      { root: container, rootMargin: '400px' });
    observer.observe(sentinel);
  }
}
