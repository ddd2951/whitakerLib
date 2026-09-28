#pragma once

#include <array>
#include <vector>

#include "facts.hpp"
#include "expand/common/reading.hpp"
#include "expand/facts/forms.hpp"
#include "types/grammar.hpp"

namespace expand {

struct Readings {
  std::array<std::vector<Reading>, facts::kLetterCount> byLetter;
};

Readings readings(const Forms& listed);

} // namespace expand
