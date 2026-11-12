#include "parse/find_readings.hpp"

#include "parse/respellings.hpp"
#include "parse/word_readings.hpp"
#include "roman.hpp"
#include "search/syncope_lookup.hpp"

#include <cstdint>
#include <cstdio>

namespace whitaker::parse {
namespace {

void addEncliticReadings(Work& out, const std::string_view word, const Stage stage) {
  const std::size_t tackonsTried = foundOrTrimmed(out) ? 1 : kEncliticTackons;
  for (const rel::Addon& tackon : rel::addonsOf(latin::AddonKind::tackon).first(tackonsTried)) {
    const std::string_view base = withoutTackon(word, tackon);
    if (base.empty())
      continue;
    const int before = out.count;
    addWordReadings(out, base, stage);
    if (stage == Stage::dictionary) {
      if (out.count == 0)
        addPrefixVariantReadings(out, base);
      addSyncopeReadings(out, base);
    }
    if (out.count != before) {
      prependAddon(out, before, tackon);
      return;
    }
  }
}

[[nodiscard]] bool anyFormOfEsse(const Work& out) {
  for (int i = 0; i < out.count; ++i) {
    const Match& match = out.matches[i];
    if (part(match) == latin::Part::V) {
      const auto e = rel::entry(match.entry);
      if (e.part == latin::Part::V && e.which == 5 && e.variant == 1)
        return true;
    }
  }
  return false;
}

void addRomanNumeral(Work& out, const std::string_view word) {
  const unsigned value = whitaker::romanValue(word);
  if (value == 0)
    return;
  Match* const added = newMatch(out);
  if (!added)
    return;

  Match& match = *added;
  match = {};
  match.part = latin::Part::NUM;
  match.entry = std::to_underlying(kNoDictionaryWord);
  match.g[0] = 2u << 4;
  match.g[2] = std::to_underlying(latin::NumeralSort::CARD);
  Change c{};
  c.frequency = latin::Frequency::X;
  setChange(out, match, c);
}

} // namespace

void addSyncopeReadings(Work& out, const std::string_view word) {
  if (anyFormOfEsse(out))
    return;
  if (const whitaker::Syncope syncope = whitaker::syncopeLookup(word); syncope.length != 0) {
    static_assert(std::to_underlying(CRoute::syncopeIi) == std::to_underlying(latin::Syncope::II) &&
                  std::to_underlying(CRoute::syncopeIs) == std::to_underlying(latin::Syncope::IS));
    const int first = out.count;
    addDictionaryReadings(out, syncope.restored());
    for (int i = first; i < out.count; ++i) {
      Change c = change(out, out.matches[i]);
      c.route = CRoute{syncope.rule};
      c.syncope = latin::Syncope{syncope.rule};
      setChange(out, out.matches[i], c);
    }
  }
}

void findReadings(Work& out, const std::string_view word) {
  addWordReadings(out, word, Stage::dictionary);
  WHITAKER_TRACE_HOOK(ran(trace::Pass::dictionary, out));
  addRomanNumeral(out, word);
  WHITAKER_TRACE_HOOK(ran(trace::Pass::romanNumeral, out));
  if (!foundOrTrimmed(out)) {
    addPrefixVariantReadings(out, word);
    WHITAKER_TRACE_HOOK(ran(trace::Pass::prefixVariants, out));
  }
  addSyncopeReadings(out, word);
  WHITAKER_TRACE_HOOK(ran(trace::Pass::syncope, out));
  addEncliticReadings(out, word, Stage::dictionary);
  WHITAKER_TRACE_HOOK(ran(trace::Pass::enclitic, out));
  if (foundOrTrimmed(out))
    return;
  // TODO: Port the fixes-only pass's syncope, where a restored spelling may take a prefix or suffix.
  addWordReadings(out, word, Stage::affixes);
  WHITAKER_TRACE_HOOK(ran(trace::Pass::dictionaryWithAffixes, out));
  if (out.count == 0) {
    addEncliticReadings(out, word, Stage::affixes);
    WHITAKER_TRACE_HOOK(ran(trace::Pass::encliticWithAffixes, out));
  }
}

} // namespace whitaker::parse
