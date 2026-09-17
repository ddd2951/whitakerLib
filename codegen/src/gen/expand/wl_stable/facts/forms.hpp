#pragma once

#include "gen/expand/wl_stable/common/spelling.hpp"
#include "gen/expand/wl_stable/fact_tag.hpp"
#include "gen/expand/wl_stable/facts/adverb_corrections.hpp"
#include "gen/expand/wl_stable/facts/sorted.hpp"

namespace wl_stable::expand {

// INFO:  Every form listed, in result order. Readings and Swept each say one
// thing about the form at the same position.
struct Forms {
  ByLetter byLetter;
};

namespace enrolled {
[[= FactTag::Fact]] Forms forms(const Sorted& sorted,
                                const AdverbCorrections& corrections);
[[= FactTag::Report]] void report(const Forms& forms);
} // namespace enrolled

} // namespace wl_stable::expand
