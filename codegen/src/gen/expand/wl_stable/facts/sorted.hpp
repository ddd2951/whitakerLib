#pragma once

#include "gen/expand/wl_stable/common/spelling.hpp"
#include "gen/expand/wl_stable/fact_tag.hpp"
#include "gen/expand/wl_stable/facts/candidates.hpp"
#include "gen/expand/wl_stable/facts/uniques.hpp"

namespace wl_stable::expand {

struct Sorted {
  ByLetter byLetter;
};

namespace enrolled {
[[= FactTag::Fact]] Sorted sorted(const Candidates& candidates,
                                  const Uniques& uniques);
}

} // namespace wl_stable::expand
