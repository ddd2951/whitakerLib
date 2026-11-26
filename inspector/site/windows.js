// INFO: The windows, as tmux's: one on screen at a time (<html data-window>), each keeping its own place.
// NOTE: The windows are listed in head.js, style.css and page.js's keys as well: CSS cannot take the list from a
//  script without an inline style, which the page's CSP does not allow.
import { scroller } from './dom.js';

export let current = null;
const places = {};
const shown = {};
const left = {};

export const onShow = (id, fn) => { shown[id] = fn; };
export const onLeave = (id, fn) => { left[id] = fn; };
// INFO: The window opens at its top the next time, not where it was (a new text).
export const toTop = id => { places[id] = 0; };

export function showWindow(id) {
  if (id !== current) {
    if (current) {
      places[current] = scroller().scrollTop;
      left[current]?.();
    }
    document.documentElement.dataset.window = id;
    for (const link of document.querySelectorAll('#status .win')) link.classList.toggle('on', link.dataset.win === id);
    current = id;
    history.replaceState(null, '', `#${id}`);
    scroller().scrollTo(0, places[id] ?? 0);
  }
  shown[id]?.();
}
