#pragma once

#include "relationship_schema.hpp"
#include <latin_generated.hpp>

namespace whitaker::two_ended {

static_assert(facts::kMaxWordCharacters < 32, "one cut bit per position, including the empty stem");

using relationship::Cell;
using relationship::Grammar;
using relationship::kQu;
using relationship::kUnique;
using relationship::Node;
using relationship::Rule;
using relationship::Stem;

// INFO: List_Sweep's last loop: the grammar as WORDS prints it.
[[nodiscard]] constexpr latin::Analysis printed(latin::Analysis g) {
  if (g.part == latin::Part::PRON && g.which.value == 1)
    g.variant = {};
  if (g.part == latin::Part::V && g.which.value == 3 && g.variant.value == 4) {
    g.which = {4};
    g.variant.value = 1;
  }
  return g;
}

} // namespace whitaker::two_ended
