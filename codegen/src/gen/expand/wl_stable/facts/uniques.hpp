#pragma once

#include "gen/expand/wl_stable/common/spelling.hpp"
#include "gen/expand/wl_stable/fact_tag.hpp"
#include "gen/expand/wl_stable/facts/slice.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::expand {

struct Uniques {
  ByLetter byLetter;
};

namespace enrolled {
[[= FactTag::Fact]] Uniques uniques(const tokenized::Sources& sources,
                                    const Slice& slice);
}

} // namespace wl_stable::expand
