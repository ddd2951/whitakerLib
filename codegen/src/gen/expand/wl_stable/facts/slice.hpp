#pragma once

#include <string>

#include "gen/expand/wl_stable/fact_tag.hpp"

namespace wl_stable::expand {

struct Slice {
  std::string letters;
};

namespace enrolled {
// INFO: The letters asked for, folded to their buckets and stated once each.
[[= FactTag::Fact]] Slice slice();
} // namespace enrolled

} // namespace wl_stable::expand
