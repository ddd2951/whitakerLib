// INFO: Run in <head>, before the page is drawn, so nothing flashes: the window the address names, whether this is a
//  touch screen (html[data-touch]), the optional windows the settings turn on, and the theme.
'use strict';
(() => {
  const html = document.documentElement;
  const OPTIONAL = { 'trace-build': ['trace', 'stats'], experiments: ['sentence'] };
  const WINDOWS = ['text', 'input', 'api', 'settings'];
  for (const [option, windows] of Object.entries(OPTIONAL)) {
    let on = false;
    try {
      on = localStorage.getItem(option) === 'on';
    } catch {
      // NOTE: No storage: off
    }
    if (on) html.setAttribute(`data-${option}`, '');
    if (on) WINDOWS.push(...windows);
  }
  const named = location.hash.slice(1);
  html.dataset.window = WINDOWS.includes(named) ? named : 'text';
  if (window.matchMedia('(pointer: coarse) and (hover: none)').matches) html.dataset.touch = '';
  document.addEventListener('DOMContentLoaded', () => {
    for (const option of Object.keys(OPTIONAL)) {
      const box = document.getElementById(option);
      box.checked = html.hasAttribute(`data-${option}`);
      box.addEventListener('change', () => {
        html.toggleAttribute(`data-${option}`, box.checked);
        try {
          localStorage.setItem(option, box.checked ? 'on' : 'off');
        } catch {
          // NOTE: Not kept: it holds until the page is closed
        }
      });
    }
  });
})();
(() => {
  const KEY = 'theme';
  const system = window.matchMedia('(prefers-color-scheme: light)');
  let theme = null;
  try {
    theme = localStorage.getItem(KEY);
  } catch {
    // NOTE: No storage (private mode, blocked): follow the system
  }
  const shown = () => theme ?? (system.matches ? 'day' : 'night');

  function apply() {
    if (theme === 'day' || theme === 'night') document.documentElement.dataset.theme = theme;
    else delete document.documentElement.dataset.theme;
    const button = document.getElementById('theme');
    if (button) {
      const day = shown() === 'day';
      button.textContent = day ? 'Day' : 'Night';
      button.title = day ? 'Switch to night mode' : 'Switch to day mode';
    }
    for (const radio of document.querySelectorAll('input[name="theme"]'))
      radio.checked = radio.value === (theme ?? 'system');
  }

  function pick(next) {
    theme = next;
    apply();
    try {
      if (theme) localStorage.setItem(KEY, theme);
      else localStorage.removeItem(KEY);
    } catch {
      // NOTE: Not kept: it holds until the page is closed
    }
  }

  apply();
  system.addEventListener('change', apply);
  document.addEventListener('DOMContentLoaded', () => {
    apply();
    document.getElementById('theme').addEventListener('click', () => pick(shown() === 'day' ? 'night' : 'day'));
    for (const radio of document.querySelectorAll('input[name="theme"]'))
      radio.addEventListener('change', () => pick(radio.value === 'system' ? null : radio.value));
  });
})();
