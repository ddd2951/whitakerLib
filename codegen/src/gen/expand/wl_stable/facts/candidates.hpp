#pragma once

#include <vector>

#include "gen/expand/wl_stable/common/spelling.hpp"
#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "gen/expand/wl_stable/fact_tag.hpp"
#include "gen/expand/wl_stable/facts/slice.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::expand {

// NOTE: A vector, not a string: the board takes the fact by move, and a
// container move keeps every view into it valid where a string's need not.
struct Candidates {
  std::vector<char> spellings;
  ByLetter byLetter;
};

namespace enrolled {
[[= FactTag::Fact]] Candidates candidates(const tokenized::Sources& sources,
                                          const Synthetics& synthetics,
                                          const Slice& slice);
}

} // namespace wl_stable::expand
