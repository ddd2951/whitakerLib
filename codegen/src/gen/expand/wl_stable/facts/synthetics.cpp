#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "gen/expand/wl_stable/facts/synthetics.hpp"
#include "types/domain.hpp"
#include "types/tokenized_sources.hpp"
#include "types/types.hpp"

wl_stable::expand::Synthetics wl_stable::expand::enrolled::synthetics() {
  return Synthetics{
      // NOTE: The second column is present and empty, not absent.
      .esse{.stem1 = "s",
            .stem2 = "",
            .stem3 = "fu",
            .stem4 = "fut",
            .grammar = tokenized::Verb{domain::Conjugation{5},
                                       domain::ConjugationVariant{1},
                                       TypeVerbKind::TO_BE},
            .age = TypeAge::X,
            .area = TypeArea::X,
            .geography = TypeGeography::X,
            .frequency = TypeFrequency::A,
            .source = TypeSource::X,
            .senses = "be; exist; (also used to form verb perfect passive "
                      "tenses) with NOM PERF PPL"},
      // NOTE: Fix_Adverb's two readings need an inflection identity; these
      // rows are it, and the join never crosses them.
      .adverbPositive{.grammar =
                          tokenized::inflected::Adverb{TypeComparison::POS},
                      .stemKey = domain::StemIndex{0},
                      .characterCount = domain::CharacterCount{1},
                      .ending = "e",
                      .age = TypeAge::X,
                      .frequency = TypeFrequency::B},
      .adverbSuperlative{
          .grammar = tokenized::inflected::Adverb{TypeComparison::SUPER},
          .stemKey = domain::StemIndex{0},
          .characterCount = domain::CharacterCount{2},
          .ending = "me",
          .age = TypeAge::X,
          .frequency = TypeFrequency::B},
  };
}
