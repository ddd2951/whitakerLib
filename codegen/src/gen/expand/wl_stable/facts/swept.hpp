#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "facts.hpp"
#include "gen/expand/wl_stable/fact_tag.hpp"
#include "gen/expand/wl_stable/facts/forms.hpp"
#include "gen/expand/wl_stable/facts/readings.hpp"

namespace wl_stable::expand {

// INFO: What List_Sweep decided for a form, and why.
enum class Fate : std::uint8_t {
  kept,
  stemNotAllowed,
  onlyArchaic,
  onlyMedieval,
  onlyUncommon,
};

// INFO: One fate per form of Forms, in the same positions.
struct Swept {
  std::array<std::vector<Fate>, facts::kLetterCount> byLetter;
};

namespace enrolled {
[[= FactTag::Fact]] Swept swept(const Forms& listed,
                                const Readings& readings);
[[= FactTag::Report]] void report(const Swept& swept);
}

} // namespace wl_stable::expand
