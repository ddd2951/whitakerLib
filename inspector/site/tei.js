// INFO: Perseus' TEI as plain text: notes and apparatus dropped,
//  one verse a line, one prose chapter a paragraph, a book's head on a line of its own. The XML's own line breaks
//  are layout: only those elements make lines.
export const TEI_REVISION = 1;
const DROP = new Set(['note', 'bibl', 'ref', 'figure', 'del', 'orig', 'sic', 'gap', 'teiHeader']);
// NOTE: NUL cannot appear in XML. It becomes a space only when dropping a boundary would join words.
const BOUNDARY = '\0';
// NOTE: Perseus files use HTML entities XML doesn't define; ligatures are kept as letters, the rest become a space.
const LIGATURES = { aelig: 'ae', AElig: 'Ae', oelig: 'oe', OElig: 'Oe' };
const flat = s => s.replace(/\s+/g, ' ').trim();

function walk(n) {
  if (n.nodeType === Node.TEXT_NODE) return n.data.replace(/\s+/g, ' ');
  if (n.nodeType !== Node.ELEMENT_NODE) return '';
  if (n.localName === 'note' && n.hasAttribute('target') && !n.textContent.trim()) return BOUNDARY;
  if (DROP.has(n.localName)) return '';
  const inner = [...n.childNodes].map(walk).join('');
  switch (n.localName) {
    case 'l': return flat(inner) + '\n';
    case 'p': return ` ${inner} `;
    case 'head': return `\n${flat(inner)}\n\n`;
    case 'q': return n.parentElement?.localName === 'q' ? BOUNDARY + inner : inner;
    case 'div':
      if (n.getAttribute('subtype') === 'section') return flat(inner) + ' ';
      return inner.includes('\n') ? inner + '\n' : flat(inner) + '\n\n';
    default: return inner;
  }
}

export function teiText(xml) {
  xml = xml.replace(/&(?!(?:amp|lt|gt|quot|apos|#\w+);)(\w+);/g, (_, e) => LIGATURES[e] ?? ' ');
  const doc = new DOMParser().parseFromString(xml, 'application/xml');
  const body = doc.getElementsByTagNameNS('*', 'body')[0];
  if (!body) throw new Error('not a TEI text');
  const text = walk(body).replace(/(?<=[\p{L}\p{M}])\0+(?=\p{L})/gu, ' ').replace(/\0/g, '');
  return text.split('\n').map(line => line.trim()).join('\n').replace(/\n{3,}/g, '\n\n').trim();
}
