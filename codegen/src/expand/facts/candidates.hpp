#pragma once

#include <vector>

#include "expand/common/spelling.hpp"
#include "expand/facts/slice.hpp"
#include "types/grammar.hpp"

namespace expand {

struct Candidates {
  std::vector<char> spellings;
  ByLetter byLetter;
};

Candidates candidates(const Slice& slice);

} // namespace expand
