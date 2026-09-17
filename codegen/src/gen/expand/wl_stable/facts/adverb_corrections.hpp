#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "facts.hpp"
#include "gen/expand/wl_stable/common/form.hpp"
#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "gen/expand/wl_stable/fact_tag.hpp"
#include "gen/expand/wl_stable/facts/sorted.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::expand {

struct AdverbCorrections {
  struct Correction {
    std::size_t after;
    Form form;
  };
  std::array<std::vector<Correction>, facts::kLetterCount> byLetter;
};

namespace enrolled {
[[= FactTag::Fact]] AdverbCorrections
adverbCorrections(const tokenized::Sources& sources,
                  const Synthetics& synthetics, const Sorted& sorted);
}

} // namespace wl_stable::expand
