#include "parse/result.hpp"
#include "roman.hpp"
#include "parse/respellings.hpp"
#include "search/image.hpp"

#include <limits>

namespace whitaker::parse {
namespace {
[[nodiscard]] const char* pattern(const Reading& r) {
  const auto code = rel::dictionary(r.entry).stems[r.stem & 3];
  return rel::imageStrings() + rel::read32(rel::imageRecord(rel::Section::StemPatterns, code >> rel::kStemLengthBits));
}
[[nodiscard]] latin::DictionaryWord dictionaryWordOf(const std::uint16_t index) {
  const rel::Entry e = rel::entry(index);
  const auto d = rel::dictionary(index);
  return {.part = e.part,
          .which = {e.which},
          .variant = {e.variant},
          .gender = e.gender,
          .kind = {e.kind},
          .comparison = e.comparison,
          .numeralSort = e.numeralSort,
          .numeralValue = {e.numeralValue.value},
          .age = d.age,
          .area = e.area,
          .geography = e.geography,
          .frequency = d.frequency,
          .source = e.source};
}

[[nodiscard]] unsigned offsetIn(const Work& w, const std::string_view spelling) {
  for (unsigned i = 0; i + spelling.size() <= w.word.size(); ++i)
    if (spelling.data() == w.word.data() + i)
      return i;
  return 0;
}

// INFO: Preserve search spelling only when the input and image cannot recover its output.
void spellingChange(const Work& w, Reading& r, Change& c, const std::string_view spelling, const unsigned length,
                    const bool raw) {
  const unsigned start = r.stem >> 2;
  const char* const p = raw ? "" : pattern(r);
  bool same = start + length <= w.word.size();
  for (unsigned i = 0; i < length; ++i) {
    const char want = *p && p[i] != '.' ? p[i] : (raw ? spelling[i] : facts::foldLetter(spelling[i]));
    c.stemText[i] = want;
    const char have =
        start + i < w.word.size()
            ? (*p && p[i] != '.' ? p[i] : (raw ? w.word[start + i] : facts::foldLetter(w.word[start + i])))
            : '\0';
    same = same && want == have;
  }
  if (!same)
    c.stemTextLength = static_cast<std::uint8_t>(length);
  else
    c.stemText = {};
  c.rawStem = raw;
  const unsigned natural = r.entry == std::to_underlying(kNoDictionaryWord)
                               ? static_cast<unsigned>(w.word.size())
                               : rel::dictionary(r.entry).stems[r.stem & 3] & ((1u << rel::kStemLengthBits) - 1);
  if (length != natural)
    c.stemLength = static_cast<std::uint8_t>(length);
  const auto ending = spelling.substr(length);
  const unsigned at = start + length;
  bool sameEnding = at + ending.size() <= w.word.size();
  for (unsigned i = 0; i < ending.size(); ++i) {
    c.endingText[i] = facts::foldLetter(ending[i]);
    sameEnding =
        sameEnding && c.endingText[i] == facts::foldLetter(w.word[std::min<std::size_t>(at + i, w.word.size() - 1)]);
  }
  if (!sameEnding)
    c.endingTextLength = static_cast<std::uint8_t>(ending.size());
  else
    c.endingText = {};
  if (at + ending.size() != w.word.size())
    c.endingLength = static_cast<std::uint8_t>(ending.size());
}
} // namespace

void setChange(Work& w, Reading& r, const Change& c) {
  static constexpr Change empty{};
  if (c == empty) {
    r.changes = 0;
    r.hasChange = false;
    return;
  }
  for (unsigned i = 0; i < w.changeCount; ++i)
    if (w.changes[i] == c) {
      r.changes = static_cast<std::uint8_t>(i + 1);
      r.hasChange = true;
      return;
    }
  if (w.changeCount == w.changes.size()) {
    // INFO: Discarded searches leave unreferenced entries. Reclaim them before reusing their byte indices.
    //  The source-derived bound leaves room for one new entry alongside every live answer group.
    bool live[256]{};
    for (int i = 0; i < w.count; ++i)
      live[w.matches[i].changes] = true;
    std::uint8_t remap[256]{};
    unsigned count = 0;
    for (unsigned i = 0; i < w.changeCount; ++i)
      if (live[i + 1]) {
        remap[i + 1] = static_cast<std::uint8_t>(++count);
        w.changes[count - 1] = w.changes[i];
      }
    for (int i = 0; i < w.count; ++i)
      w.matches[i].changes = remap[w.matches[i].changes];
    w.changeCount = count;
  }
  assert(w.changeCount < w.changes.size());
  w.changes[w.changeCount] = c;
  r.changes = static_cast<std::uint8_t>(++w.changeCount);
  r.hasChange = true;
}

std::uint8_t stemLength(const Reading& r, const Change& c) {
  return c.stemLength != 255 ? c.stemLength
                             : static_cast<std::uint8_t>(rel::dictionary(r.entry).stems[r.stem & 3] &
                                                         ((1u << rel::kStemLengthBits) - 1));
}
std::uint8_t endingLength(const Work& w, const Reading& r) {
  const Change& c = change(w, r);
  return c.endingLength != 255 ? c.endingLength
                               : static_cast<std::uint8_t>(w.word.size() - (r.stem >> 2) - stemLength(r, c));
}

Match* newMatch(Work& out) {
  if (out.count == out.capacity) {
    const int capacity = out.capacity == 0 ? 64 : out.capacity * 2;
    auto* const matches =
        static_cast<Match*>(std::realloc(out.matches, static_cast<std::size_t>(capacity) * sizeof(Match)));
    if (!matches) {
      out.outOfMemory = true;
      return nullptr;
    }
    out.matches = matches;
    out.capacity = capacity;
  }
  return &out.matches[out.count++];
}

void fillFromImage(Work& out, Match& m, const std::string_view spelling, const rel::Result& r) {
  m = {};
  m.part = r.grammar.part;
  m.stemKey = static_cast<std::uint8_t>(r.stemKey & 7);
  m.entry = r.dictionary;
  m.stem = static_cast<std::uint8_t>(r.stemColumn | (offsetIn(out, spelling) << 2));
  const bool fits = latin::packed::packGrammar(r.grammar, m.g);
  assert(fits);
  (void)fits;
  Change c{};
  char folded[kMaxWordLength];
  const auto ending = spelling.substr(r.stemLength);
  std::ranges::transform(ending, folded, facts::foldLetter);
  const auto labels = rel::inflectionLabels(r.description, {folded, ending.size()});
  c.age = labels.first;
  c.frequency = labels.second;
  if (r.grammar.part == latin::Part::adverb && rel::entry(r.dictionary).part == latin::Part::adjective) {
    c.route = CRoute::adverb;
    c.adverb = true;
  }
  spellingChange(out, m, c, spelling, r.stemLength, false);
  setChange(out, m, c);
  WHITAKER_TRACE_HOOK(source(m, r));
}

void addDictionaryReadings(Work& out, const std::string_view spelling, bool (*const accept)(const Reading&)) {
  const rel::Program p = rel::lookup(spelling);
  out.trimmed |= p.trimmed;
  auto cursor = p.begin;
  std::uint16_t lexeme = 0xffff;
  for (unsigned i = 0; i < p.count; ++i) {
    Match m{};
    std::uint16_t description;
    rel::nextReading(cursor, lexeme, m, description);
    if (accept && !accept(m))
      continue;
    const auto length = stemLength(m, Change{});
    const auto labels = rel::readingLabels(cursor, description, spelling, length);
    const bool adverb = part(m) == latin::Part::adverb && rel::entry(m.entry).part == latin::Part::adjective;
    const bool rewritten = spelling.data() != out.word.data() || spelling.size() != out.word.size();
    if (rewritten)
      m.stem |= static_cast<std::uint8_t>(offsetIn(out, spelling) << 2);
    if (labels.first != latin::Age::X || labels.second != latin::Frequency::A || adverb || rewritten) {
      Change c{};
      c.age = labels.first;
      c.frequency = labels.second;
      if (adverb) {
        c.route = CRoute::adverb;
        c.adverb = true;
      }
      if (rewritten)
        spellingChange(out, m, c, spelling, length, false);
      setChange(out, m, c);
    }
#ifdef WHITAKER_TRACE
    m.tags.dictionary = m.entry;
    m.tags.description = description;
#endif
    Match* const added = newMatch(out);
    if (!added)
      return;
    *added = m;
  }
}

void prependAddon(Work& out, const int first, const rel::Addon& addon) {
  for (int i = first; i < out.count; ++i) {
    Change c = change(out, out.matches[i]);
    assert(c.addonCount < c.addons.size());
    std::move_backward(c.addons.begin(), c.addons.begin() + c.addonCount, c.addons.begin() + c.addonCount + 1);
    c.addons[0] = addon.id;
    ++c.addonCount;
    setChange(out, out.matches[i], c);
  }
}
void addAddonReading(Work& out, const std::string_view word, const Reading& r, const std::uint8_t orthLength,
                     const std::uint16_t inflect, const std::uint16_t addon, const std::uint16_t secondAddon) {
  Match* const m = newMatch(out);
  if (!m)
    return;
  *m = {};
  static_cast<Reading&>(*m) = r;
  const auto inflection = rel::inflection(inflect);
  Change c{};
  c.age = inflection.age;
  c.frequency = inflection.frequency;
  c.addonCount = secondAddon == kNoAddon ? 1 : 2;
  c.addons[0] = addon;
  c.addons[1] = secondAddon == kNoAddon ? 0 : secondAddon;
  const unsigned localOffset = r.stem >> 2;
  const unsigned offset = localOffset + offsetIn(out, word);
  m->stem = static_cast<std::uint8_t>((r.stem & 3) | (offset << 2));
  const auto orth = word.substr(localOffset, orthLength);
  const std::string_view ending = rel::imageStrings() + inflection.ending;
  // NOTE: No stem or ending is rendered for affix candidates; only an unrecoverable spelling is retained.
  c.rawStem = true;
  const auto natural = stemLength(r, Change{});
  if (orthLength != natural)
    c.stemLength = orthLength;
  if (offset + orth.size() + ending.size() != out.word.size())
    c.endingLength = static_cast<std::uint8_t>(ending.size());
  if (offset + orth.size() > out.word.size() || orth != out.word.substr(offset, orthLength)) {
    c.stemTextLength = orthLength;
    std::ranges::copy(orth, c.stemText.begin());
  }
  const unsigned at = offset + orthLength;
  bool same = at + ending.size() <= out.word.size();
  for (unsigned i = 0; i < ending.size(); ++i)
    same = same && ending[i] == facts::foldLetter(out.word[at + i]);
  if (!same) {
    c.endingTextLength = static_cast<std::uint8_t>(ending.size());
    std::ranges::copy(ending, c.endingText.begin());
  }
  setChange(out, *m, c);
}

// INFO: The public route: the five ways, the syncope's rule left to Derivation.
[[nodiscard]] static latin::Route publicRoute(const CRoute route) {
  switch (route) {
    case CRoute::direct:
      return latin::Route::direct;
    case CRoute::syncopeIi:
    case CRoute::syncopeS:
    case CRoute::syncopeR:
    case CRoute::syncopeIer:
    case CRoute::syncopeIs:
      return latin::Route::syncope;
    case CRoute::adverb:
      return latin::Route::adverb;
    case CRoute::slur:
      return latin::Route::slur;
    case CRoute::wordMod:
      return latin::Route::wordMod;
  }
  return latin::Route::direct;
}

// INFO: The key of a trimmed rare reading, which comes without one, from its class.
[[nodiscard]] static std::uint8_t searchedStemKey(const Reading& r, const ReadingDetail& d) {
  const auto endings = rel::endingRun(d.form.ending);
  const auto rows = rel::rows(rel::dictionaryClass(rel::dictionary(r.entry).classId & rel::kClassIdMask));
  for (auto i = rows.first; i < rel::end(rows); ++i) {
    const auto row = rel::classRow(i);
    if (row.inflect >= endings.first && row.inflect < rel::end(endings) &&
        rel::grammar(row.description) == d.form.grammar)
      return rel::inflection(row.inflect).stemKey;
  }
  return 0;
}

void decode(const std::string_view word, const Reading& r, const Change& c, ReadingDetail& d) {
  d = {};
  const unsigned start = r.stem >> 2;
  const unsigned length =
      r.entry == std::to_underlying(kNoDictionaryWord) ? static_cast<unsigned>(word.size()) : stemLength(r, c);
  const unsigned el = c.endingLength != 255 ? c.endingLength : static_cast<unsigned>(word.size()) - start - length;
  if (c.stemTextLength)
    std::copy_n(c.stemText.begin(), c.stemTextLength, d.form.stemSpelling);
  else if (r.entry == std::to_underlying(kNoDictionaryWord) || c.rawStem)
    std::copy_n(word.data() + start, length, d.form.stemSpelling);
  else
    rel::printStem(word.substr(start), static_cast<std::uint8_t>(length), pattern(r), d.form.stemSpelling);
  if (c.endingTextLength)
    std::copy_n(c.endingText.begin(), c.endingTextLength, d.form.ending);
  else
    for (unsigned i = 0; i < el; ++i)
      d.form.ending[i] = facts::foldLetter(word[start + length + i]);
  if (r.entry == std::to_underlying(kNoDictionaryWord)) {
    const auto value = whitaker::romanValue(word);
    d.dictionaryWord = {.part = latin::Part::numeral,
                        .which = {2},
                        .numeralSort = latin::NumeralSort::cardinal,
                        .numeralValue = {static_cast<std::uint16_t>(value)}};
    d.dictionaryWordId = kNoDictionaryWord;
    d.meaning = "as a ROMAN NUMERAL;";
  } else {
    d.dictionaryWord = dictionaryWordOf(r.entry);
    d.dictionaryWordId = DictionaryWordId{r.entry};
    d.meaning = rel::dictionaryMeaning(r.entry);
    while (*d.meaning == '|')
      ++d.meaning;
    while (*d.meaning == ' ')
      ++d.meaning;
  }
  d.partName = latin::name(part(r));
  d.form.grammar = grammar(r);
  d.form.inflectionAge = c.age;
  d.form.inflectionFrequency = c.frequency;
  d.form.derivation = Derivation{.syncope = c.syncope, .adverb = c.adverb};
  d.form.spellingRuleId = c.spellingRule;
  // INFO: A unique and a runtime adverb (Fix_Adverb's rows) have no key; a trimmed rare reading has one to find.
  if (r.entry != std::to_underlying(kNoDictionaryWord) && !c.rawStem)
    d.stemKey =
        r.stemKey != 0 || r.entry >= std::to_underlying(kFirstUnique) || c.adverb ? r.stemKey : searchedStemKey(r, d);
  d.addonCount = c.addonCount;
  d.route = publicRoute(c.route);
  for (unsigned i = 0; i < c.addonCount; ++i)
    d.addons[i] = {c.addons[i], rel::addon(c.addons[i]).kind};
}

enum Flag : unsigned {
  flagAddons = 1,
  flagAdverb = 4,
  flagRule = 8,
  flagLabels = 16,
  flagEndingLength = 32,
  flagStemText = 64,
  flagEndingText = 128,
  flagStemLength = 256,
  flagRoute = 512,
};
[[nodiscard]] static CRoute derivedRoute(const Change& c) {
  if (c.adverb)
    return CRoute::adverb;
  if (c.syncope != latin::Syncope{})
    return CRoute(std::to_underlying(c.syncope));
  if (c.spellingRule)
    return parse::spellingRule(c.spellingRule).slur ? CRoute::slur : CRoute::wordMod;
  return CRoute::direct;
}
unsigned encodeChange(const Change& c, unsigned char* const out) {
  const unsigned flags = (c.addonCount ? flagAddons : 0u) | (unsigned(std::to_underlying(c.syncope)) << 10) |
                         (c.adverb ? flagAdverb : 0u) | (c.spellingRule ? flagRule : 0u) |
                         (c.age != latin::Age::X || c.frequency != latin::Frequency::A ? flagLabels : 0u) |
                         (c.stemLength != 255 ? flagStemLength : 0u) | (c.endingLength != 255 ? flagEndingLength : 0u) |
                         (c.stemTextLength ? flagStemText : 0u) | (c.endingTextLength ? flagEndingText : 0u) |
                         (c.route != derivedRoute(c) ? flagRoute : 0u);
  unsigned n = 0;
  const auto put = [&](const unsigned x) { out[n++] = static_cast<unsigned char>(x); };
  put(flags);
  put(flags >> 8);
  if (flags & flagAddons) {
    put(c.addonCount);
    for (unsigned i = 0; i < c.addonCount; ++i) {
      put(c.addons[i]);
      put(c.addons[i] >> 8);
    }
  }
  if (flags & flagRule)
    put(c.spellingRule);
  if (flags & flagLabels) {
    put((unsigned(std::to_underlying(c.age)) << 4) | std::to_underlying(c.frequency));
  }
  if (flags & flagStemLength)
    put(c.stemLength);
  if (flags & flagEndingLength)
    put(c.endingLength);
  if (flags & flagStemText) {
    put(c.stemTextLength);
    for (unsigned i = 0; i < c.stemTextLength; ++i)
      put(static_cast<unsigned char>(c.stemText[i]));
  }
  if (flags & flagEndingText) {
    put(c.endingTextLength);
    for (unsigned i = 0; i < c.endingTextLength; ++i)
      put(static_cast<unsigned char>(c.endingText[i]));
  }
  if (flags & flagRoute)
    put(std::to_underlying(c.route));
  return n;
}
Change decodeChange(const unsigned char* in) {
  Change c{};
  const unsigned flags = in[0] | unsigned(in[1]) << 8;
  in += 2;
  c.adverb = flags & flagAdverb;
  c.syncope = latin::Syncope((flags >> 10) & 7);
  if (flags & flagAddons) {
    c.addonCount = *in++;
    for (unsigned i = 0; i < c.addonCount; ++i) {
      c.addons[i] = static_cast<std::uint16_t>(in[0] | unsigned(in[1]) << 8);
      const auto kind = rel::addon(c.addons[i]).kind;
      c.rawStem = c.rawStem || kind == latin::AddonKind::prefix || kind == latin::AddonKind::suffix ||
                  kind == latin::AddonKind::packon;
      in += 2;
    }
  }
  if (flags & flagRule)
    c.spellingRule = *in++;
  if (flags & flagLabels) {
    c.age = latin::Age(*in >> 4);
    c.frequency = latin::Frequency(*in++ & 15);
  }
  if (flags & flagStemLength)
    c.stemLength = *in++;
  if (flags & flagEndingLength)
    c.endingLength = *in++;
  if (flags & flagStemText) {
    c.stemTextLength = *in++;
    std::copy_n(in, c.stemTextLength, c.stemText.begin());
    in += c.stemTextLength;
  }
  if (flags & flagEndingText) {
    c.endingTextLength = *in++;
    std::copy_n(in, c.endingTextLength, c.endingText.begin());
    in += c.endingTextLength;
  }
  c.route = flags & flagRoute ? CRoute{*in++} : derivedRoute(c);
  return c;
}
} // namespace whitaker::parse
