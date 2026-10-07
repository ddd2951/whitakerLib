#pragma once

#include "expand/common/spelling.hpp"
#include "expand/facts/adverb_corrections.hpp"
#include "expand/facts/sorted.hpp"

namespace expand {

// Readings and Swept line up with Forms by position.
struct Forms {
  ByLetter byLetter;
};

Forms forms(const Sorted& sorted, const AdverbCorrections& corrections);

} // namespace expand
