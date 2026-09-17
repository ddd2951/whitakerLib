#pragma once

#include <array>
#include <vector>

#include "facts.hpp"
#include "gen/expand/wl_stable/common/reading.hpp"
#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "gen/expand/wl_stable/fact_tag.hpp"
#include "gen/expand/wl_stable/facts/forms.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::expand {

// INFO: One reading per form of Forms, in the same positions.
struct Readings {
  std::array<std::vector<Reading>, facts::kLetterCount> byLetter;
};

namespace enrolled {
[[= FactTag::Fact]] Readings readings(const tokenized::Sources& sources,
                                      const Synthetics& synthetics,
                                      const Forms& listed);
}

} // namespace wl_stable::expand
