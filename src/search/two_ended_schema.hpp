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

// INFO: A back-head run in 16 bits: its first in the low kHeadRunFirstBits, its count above.
inline constexpr unsigned kHeadRunFirstBits = 9, kHeadRunCountBits = 7;
static_assert(kHeadRunFirstBits + kHeadRunCountBits == 16);

// INFO: List_Sweep's last loop: the grammar as WORDS prints it.
[[nodiscard]] constexpr latin::Analysis printed(latin::Analysis g) {
  if (g.part == latin::Part::pronoun && g.which.value == 1)
    g.variant = {};
  if (g.part == latin::Part::verb && g.which.value == 3 && g.variant.value == 4) {
    g.which = {4};
    g.variant.value = 1;
  }
  return g;
}

} // namespace whitaker::two_ended
