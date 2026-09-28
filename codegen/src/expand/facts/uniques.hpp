#pragma once

#include "expand/common/spelling.hpp"
#include "expand/facts/slice.hpp"
#include "types/grammar.hpp"

namespace expand {

struct Uniques {
  ByLetter byLetter;
};

Uniques uniques(const Slice& slice);

} // namespace expand
