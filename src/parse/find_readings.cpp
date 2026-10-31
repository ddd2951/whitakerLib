#include "parse/find_readings.hpp"

#include "parse/respellings.hpp"
#include "parse/word_readings.hpp"
#include "roman.hpp"
#include "search/syncope_lookup.hpp"

#include <cstdint>
#include <cstdio>

namespace whitaker::parse {
namespace {

void addEncliticReadings(Work& out, Text& text, std::string_view word, Stage stage) noexcept {
  const std::size_t tackonsTried = foundOrTrimmed(out) ? 1 : kEncliticTackons;
  for (const rel::Addon& tackon : rel::addonsOf(latin::AddonKind::Tackon).first(tackonsTried)) {
    const std::string_view base = withoutTackon(word, tackon);
    if (base.empty())
      continue;
    const int before = out.count;
    addWordReadings(out, text, base, stage);
    if (stage == Stage::Dictionary) {
      if (out.count == 0)
        addPrefixVariantReadings(out, text, base);
      addSyncopeReadings(out, text, base);
    }
    if (out.count != before) {
      prependAddon(out, before, tackon);
      return;
    }
  }
}

[[nodiscard]] bool anyFormOfEsse(const Work& out) noexcept {
  for (int i = 0; i < out.count; ++i) {
    const Match& match = out.matches[i];
    if (match.grammar.part == WHITAKER_PART_V && match.entry.part == WHITAKER_PART_V && match.entry.which == 5 &&
        match.entry.variant == 1)
      return true;
  }
  return false;
}

void addRomanNumeral(Work& out, Text& text, std::string_view word) noexcept {
  const unsigned value = whitaker::romanValue(word);
  if (value == 0)
    return;
  Match* added = newMatch(out);
  if (!added)
    return;

  char meaning[kRomanMeaningBytes];
  std::snprintf(meaning, sizeof meaning, " %u  as a ROMAN NUMERAL;", value);
  Match& match = *added;
  match = {};
  match.orth = text.keep(word);
  match.ending = "";
  match.meaning = text.keep(meaning);
  match.pos = latin::name(latin::Part::NUM);
  match.grammar = grammarOf({.part = latin::Part::NUM, .which = 2, .numeralSort = latin::NumeralSort::CARD});
  match.entry.part = WHITAKER_PART_NUM;
  match.entry.which = 2;
  match.entry.numeral_sort = WHITAKER_NUMERAL_SORT_CARD;
  // NOTE: Below 5000, past WORDS' 0..1000 numeral values.
  match.entry.numeral_value = static_cast<std::uint16_t>(value);
  match.entry.id = WHITAKER_NO_RECORD;
}

} // namespace

void addSyncopeReadings(Work& out, Text& text, std::string_view word) noexcept {
  if (anyFormOfEsse(out))
    return;
  if (const whitaker::Syncope syncope = whitaker::syncopeLookup(word); syncope.length != 0) {
    static_assert(WHITAKER_ROUTE_SYNCOPE_II == 1 && WHITAKER_ROUTE_SYNCOPE_IS == 5, "syncope rules are WhitakerRoutes");
    const int first = out.count;
    addDictionaryReadings(out, text, syncope.restored());
    for (int i = first; i < out.count; ++i)
      out.matches[i].route = syncope.rule;
  }
}

void findReadings(Work& out, Text& text, std::string_view word) noexcept {
  addWordReadings(out, text, word, Stage::Dictionary);
  addRomanNumeral(out, text, word);
  if (!foundOrTrimmed(out))
    addPrefixVariantReadings(out, text, word);
  addSyncopeReadings(out, text, word);
  addEncliticReadings(out, text, word, Stage::Dictionary);
  if (foundOrTrimmed(out))
    return;
  // FIXME: Not ported: the fixes-only pass's syncope, where a restored spelling may take a prefix or suffix.
  addWordReadings(out, text, word, Stage::Affixes);
  if (out.count == 0)
    addEncliticReadings(out, text, word, Stage::Affixes);
}

} // namespace whitaker::parse
