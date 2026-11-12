#include "execution_steps/steps.hpp"

#include "storage/inflects.hpp"

void execution_steps::inflects::addAdverbInflections() {
  using storage::inflects;
  using word::kAdverbPositive;
  using word::kAdverbSuperlative;
  // NOTE: Fix_Adverb's two readings need an inflection identity; these rows are it, and the join never crosses them.
  inflects[kAdverbPositive.value] = {.grammar = latin::inflected::Adverb{latin::Comparison::POS},
                                     .stemKey = latin::StemKey{0},
                                     .characterCount = latin::CharacterCount{1},
                                     .ending = "e",
                                     .age = latin::Age::X,
                                     .frequency = latin::Frequency::B};
  inflects[kAdverbSuperlative.value] = {.grammar = latin::inflected::Adverb{latin::Comparison::SUPER},
                                        .stemKey = latin::StemKey{0},
                                        .characterCount = latin::CharacterCount{2},
                                        .ending = "me",
                                        .age = latin::Age::X,
                                        .frequency = latin::Frequency::B};
}
