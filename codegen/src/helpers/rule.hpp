#pragma once

#include "helpers/intern.hpp"
#include "expand/common/reading.hpp"
#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "search/relationship_schema.hpp"

#include <string>
#include <utility>
#include <variant>

namespace helpers {

namespace rel = whitaker::relationship;

inline unsigned rule(const expand::Origin origin, const unsigned sourceRow, std::string suffix, std::string ending,
                     const int correction) {
  const auto r = expand::readingOf(origin);
  const auto grammar = emitter::analysisOf(r.entry, r.inflection, r.key);
  auto sweep = grammar;
  if (const auto* const p = std::get_if<latin::Pronoun>(&r.entry); p && p->declension.value == 1 && !r.unique)
    sweep.variant.value = std::to_underlying(p->pronounKind);
  // INFO: A rule's key is its INFLECTS row's stem key; a unique's rule has none.
  return intern({sourceRow, std::move(suffix), std::move(ending), rel::packGrammar(grammar), rel::packGrammar(sweep),
                 r.unique ? 0u : r.key.value, std::to_underlying(r.inflectionAge),
                 std::to_underlying(r.inflectionFrequency), correction});
}

} // namespace helpers
