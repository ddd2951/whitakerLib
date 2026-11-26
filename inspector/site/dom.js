// INFO: Small helpers every part of the page uses.
export const $ = id => document.getElementById(id);
export const seconds = ms => `${(ms / 1000).toFixed(2)} s`;
export const count = n => n.toLocaleString('en');
export const clock = text => { $('clock').textContent = text; };
// INFO: What scrolls the windows: the page, or on a touch screen the box between the status line's two rows (#scroll,
//  style.css), so the page itself stays still and the browser's bars, which come and go as a page scrolls, can't
//  move the line.
export const scroller = () => ('touch' in document.documentElement.dataset ? $('scroll') : document.scrollingElement);
// INFO: Where the reading area starts on screen: under the status line's text row.
export const readingTop = () => document.querySelector('#status .line').getBoundingClientRect().bottom;

export function element(tag, className, text) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (text !== undefined) node.textContent = text;
  return node;
}
