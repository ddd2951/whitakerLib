#include "parse/adverb.hpp"

#include "parse/result.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <utility>

namespace whitaker::parse {
namespace {

[[nodiscard]] bool makesAnAdverb(const Match& m) noexcept {
  const WhitakerGrammar& g = m.grammar;
  const bool positive = g.comparison == WHITAKER_COMPARISON_POS && g.which == 1 && g.variant == 1;
  return g.part == WHITAKER_PART_ADJ && g.case_of == WHITAKER_CASE_VOC && g.number == WHITAKER_NUMBER_S &&
         g.gender == WHITAKER_GENDER_M && (positive || g.comparison == WHITAKER_COMPARISON_SUPER) &&
         m.entry.id < WHITAKER_GENERAL_RECORDS;
}

[[nodiscard]] Match adverbOf(const Match& adjective) noexcept {
  Match adverb = adjective;
  adverb.grammar = grammarOf({.part = latin::Part::ADV, .comparison = latin::Comparison{adjective.grammar.comparison}});
  adverb.pos = latin::name(latin::Part::ADV);
  adverb.route = WHITAKER_ROUTE_ADVERB;
  adverb.inflectionAge = std::to_underlying(latin::Age::X);
  adverb.inflectionFrequency = std::to_underlying(latin::Frequency::B);
  return adverb;
}

[[nodiscard]] bool sameAddons(const Match& a, const Match& b) noexcept {
  return std::ranges::equal(
      std::span{a.addons, a.addonCount}, std::span{b.addons, b.addonCount},
      [](const WhitakerAddonStep& x, const WhitakerAddonStep& y) { return x.id == y.id && x.kind == y.kind; });
}

[[nodiscard]] int adjectiveRunStart(const Work& out, int end) noexcept {
  int begin = end - 1;
  while (begin > 0 && out.matches[begin - 1].grammar.part == WHITAKER_PART_ADJ &&
         sameAddons(out.matches[begin - 1], out.matches[end - 1]))
    --begin;
  return begin;
}

[[nodiscard]] bool alreadyMade(const Work& out, int firstMade, const Match& adjective) noexcept {
  return std::any_of(out.matches + firstMade, out.matches + out.count, [&](const Match& adverb) {
    return adverb.entry.id == adjective.entry.id && adverb.grammar.comparison == adjective.grammar.comparison;
  });
}

// NOTE: The image made the adverbs of its own readings; these are the readings made at run time.
void addAdverbOfEachAdjective(Work& out) noexcept {
  if (std::any_of(out.matches, out.matches + out.count,
                  [](const Match& m) { return m.grammar.part == WHITAKER_PART_ADV; }))
    return;
  const int firstMade = out.count;
  for (int end = out.count; end > 0;) {
    if (out.matches[end - 1].grammar.part != WHITAKER_PART_ADJ) {
      --end;
      continue;
    }
    const int begin = adjectiveRunStart(out, end);
    for (int i = begin; i < end; ++i) {
      // NOTE: A copy: adding a reading may move them.
      const Match adjective = out.matches[i];
      if (!makesAnAdverb(adjective) || alreadyMade(out, firstMade, adjective))
        continue;
      Match* added = newMatch(out);
      if (!added)
        return;
      *added = adverbOf(adjective);
    }
    end = begin;
  }
}

void listAdverbsLast(Work& out) noexcept {
  std::stable_partition(out.matches, out.matches + out.count,
                        [](const Match& m) { return m.route != WHITAKER_ROUTE_ADVERB; });
}

} // namespace

void addAdverbsOfAdjectives(Work& out) noexcept {
  addAdverbOfEachAdjective(out);
  listAdverbsLast(out);
}

} // namespace whitaker::parse
