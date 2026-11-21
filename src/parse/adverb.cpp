#include "parse/adverb.hpp"

#include "parse/result.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <utility>

namespace whitaker::parse {
namespace {

[[nodiscard]] bool makesAnAdverb(const Match& m) {
  const auto g = grammar(m);
  const bool positive = g.comparison == latin::Comparison::positive && g.which.value == 1 && g.variant.value == 1;
  return g.part == latin::Part::adjective && g.caseOf == latin::Case::vocative && g.number == latin::Number::singular &&
         g.gender == latin::Gender::masculine && (positive || g.comparison == latin::Comparison::superlative) &&
         m.entry < std::to_underlying(kFirstUnique);
}

[[nodiscard]] Match adverbOf(Work& out, const Match& adjective) {
  Match adverb = adjective;
  adverb.part = latin::Part::adverb;
  adverb.hasChange = false;
  adverb.stemKey = 0;
  adverb.g[0] = adverb.g[1] = 0;
  adverb.g[2] = std::to_underlying(grammar(adjective).comparison);
  Change c = change(out, adjective);
  c.route = CRoute::adverb;
  c.adverb = true;
  c.age = latin::Age::X;
  c.frequency = latin::Frequency::B;
  setChange(out, adverb, c);
  return adverb;
}

[[nodiscard]] bool sameAddons(const Work& out, const Match& a, const Match& b) {
  const Change& x = change(out, a);
  const Change& y = change(out, b);
  return x.addonCount == y.addonCount &&
         std::equal(x.addons.begin(), x.addons.begin() + x.addonCount, y.addons.begin());
}

[[nodiscard]] int adjectiveRunStart(const Work& out, const int end) {
  int begin = end - 1;
  while (begin > 0 && part(out.matches[begin - 1]) == latin::Part::adjective &&
         sameAddons(out, out.matches[begin - 1], out.matches[end - 1]))
    --begin;
  return begin;
}

[[nodiscard]] bool alreadyMade(const Work& out, const int firstMade, const Match& adjective) {
  return std::any_of(out.matches + firstMade, out.matches + out.count, [&](const Match& adverb) {
    return adverb.entry == adjective.entry && grammar(adverb).comparison == grammar(adjective).comparison;
  });
}

// NOTE: The image made the adverbs of its own readings; these are the readings made at run time.
void addAdverbOfEachAdjective(Work& out) {
  if (std::any_of(out.matches, out.matches + out.count, [](const Match& m) { return part(m) == latin::Part::adverb; }))
    return;
  const int firstMade = out.count;
  for (int end = out.count; end > 0;) {
    if (part(out.matches[end - 1]) != latin::Part::adjective) {
      --end;
      continue;
    }
    const int begin = adjectiveRunStart(out, end);
    for (int i = begin; i < end; ++i) {
      // NOTE: A copy: adding a reading may move them.
      const Match adjective = out.matches[i];
      if (!makesAnAdverb(adjective) || alreadyMade(out, firstMade, adjective))
        continue;
      Match* const added = newMatch(out);
      if (!added)
        return;
      *added = {};
      *added = adverbOf(out, adjective);
    }
    end = begin;
  }
}

void listAdverbsLast(Work& out) {
  std::stable_partition(out.matches, out.matches + out.count,
                        [&out](const Match& m) { return route(out, m) != CRoute::adverb; });
}

} // namespace

void addAdverbsOfAdjectives(Work& out) {
  unsigned parts = 0;
  for (int i = 0; i < out.count; ++i)
    parts |= 1u << std::to_underlying(part(out.matches[i]));
  if (!(parts & ((1u << std::to_underlying(latin::Part::adjective)) | (1u << std::to_underlying(latin::Part::adverb)))))
    return;
  addAdverbOfEachAdjective(out);
  listAdverbsLast(out);
}

} // namespace whitaker::parse
