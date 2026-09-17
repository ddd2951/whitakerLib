#include <cstddef>
#include <type_traits>
#include <variant>

#include "gen/expand/wl_stable/common/reading.hpp"
#include "gen/expand/wl_stable/common/rows.hpp"
#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "gen/expand/wl_stable/common/tables.hpp"

namespace {

using namespace wl_stable::expand;

struct Halves {
  tokenized::Grammar entry;
  tokenized::Inflection inflection;
};

Halves split(const tokenized::UniqueGrammar& grammar) {
  return std::visit(
      [](const auto& u) -> Halves {
        using U = std::remove_cvref_t<decltype(u)>;
        if constexpr (std::is_same_v<U, tokenized::unique::Noun>)
          return {tokenized::Noun{u.declension, u.declensionVariant, u.gender,
                                  u.nounKind},
                  tokenized::inflected::Noun{u.declension, u.declensionVariant,
                                             u.caseOf, u.number, u.gender}};
        else if constexpr (std::is_same_v<U, tokenized::unique::Pronoun>)
          return {tokenized::Pronoun{u.declension, u.declensionVariant,
                                     u.pronounKind},
                  tokenized::inflected::Pronoun{u.declension,
                                                u.declensionVariant, u.caseOf,
                                                u.number, u.gender}};
        else if constexpr (std::is_same_v<U, tokenized::unique::Adjective>)
          return {tokenized::Adjective{u.declension, u.declensionVariant,
                                       u.comparison},
                  tokenized::inflected::Adjective{
                      u.declension, u.declensionVariant, u.caseOf, u.number,
                      u.gender, u.comparison}};
        else {
          static_assert(std::is_same_v<U, tokenized::unique::Verb>);
          return {
              tokenized::Verb{u.conjugation, u.conjugationVariant, u.verbKind},
              tokenized::inflected::Verb{u.conjugation, u.conjugationVariant,
                                         u.tense, u.voice, u.mood, u.person,
                                         u.number}};
        }
      },
      grammar);
}

Reading fromJoined(const tokenized::Sources& sources,
                   const Synthetics& synthetics, const Joined& j) {
  const DictlineEntry d = entry(sources, synthetics, j.entry);
  const InflectsEntry f = inflection(sources, synthetics, j.inflection);
  return Reading{stemAt(d, j.column),
                 f.stemKey,
                 d.grammar,
                 f.grammar,
                 d.age,
                 d.area,
                 d.geography,
                 d.frequency,
                 d.source,
                 f.age,
                 f.frequency,
                 d.senses,
                 false};
}

Reading fromUnique(const tokenized::Sources& sources, UniquesRow row) {
  const tokenized::Uniques& u = sources.uniques;
  const std::size_t i = row.value;
  const Halves halves = split(u.grammar[i]);
  return Reading{u.form[i],
                 domain::StemIndex{1},
                 halves.entry,
                 halves.inflection,
                 u.age[i],
                 u.area[i],
                 u.geography[i],
                 u.frequency[i],
                 u.source[i],
                 TypeAge::X,
                 TypeFrequency::X,
                 u.senses[i],
                 true};
}

Reading fromUniqueAdverb(const tokenized::Sources& sources,
                         const Synthetics& synthetics, const UniqueAdverb& a) {
  Reading reading = fromUnique(sources, a.entry);
  const InflectsEntry f = inflection(sources, synthetics, a.inflection);
  reading.key = f.stemKey;
  reading.inflection = f.grammar;
  reading.inflectionAge = f.age;
  reading.inflectionFrequency = f.frequency;
  return reading;
}

} // namespace

wl_stable::expand::Reading
wl_stable::expand::readingOf(const tokenized::Sources& sources,
                             const Synthetics& synthetics,
                             const Origin& origin) {
  return std::visit(
      [&](const auto& o) -> Reading {
        using O = std::remove_cvref_t<decltype(o)>;
        if constexpr (std::is_same_v<O, Joined>)
          return fromJoined(sources, synthetics, o);
        else if constexpr (std::is_same_v<O, Unique>)
          return fromUnique(sources, o.entry);
        else
          return fromUniqueAdverb(sources, synthetics, o);
      },
      origin);
}
