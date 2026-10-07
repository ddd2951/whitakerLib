#pragma once

#include "expand/common/spelling.hpp"
#include "types/grammar.hpp"

namespace expand {

struct Uniques {
  ByLetter byLetter;
};

Uniques uniques();

} // namespace expand
