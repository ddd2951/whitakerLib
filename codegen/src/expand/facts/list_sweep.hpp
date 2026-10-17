#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "facts.hpp"
#include "expand/facts/forms.hpp"
#include "expand/facts/readings.hpp"

namespace expand {

enum class Fate : std::uint8_t {
  kept,
  stemNotAllowed,
  onlyArchaic,
  onlyMedieval,
  onlyUncommon,
  duplicate,
};

struct Swept {
  std::array<std::vector<Fate>, facts::kLetterCount> byLetter;
};

// INFO: List_Sweep per spelling; the kept forms come first.
Swept listSweep(Forms& listed, Readings& readings);

} // namespace expand
