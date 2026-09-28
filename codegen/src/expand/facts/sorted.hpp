#pragma once

#include "expand/common/spelling.hpp"
#include "expand/facts/candidates.hpp"
#include "expand/facts/uniques.hpp"

namespace expand {

struct Sorted {
  ByLetter byLetter;
};

Sorted sorted(const Candidates& candidates, const Uniques& uniques);

} // namespace expand
