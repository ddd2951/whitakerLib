#pragma once

#include <vector>

#include "expand/common/spelling.hpp"
#include "types/grammar.hpp"

namespace expand {

struct Candidates {
  std::vector<char> spellings;
  ByLetter byLetter;
};

Candidates candidates();

} // namespace expand
