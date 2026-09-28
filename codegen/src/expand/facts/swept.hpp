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
};

struct Swept {
  std::array<std::vector<Fate>, facts::kLetterCount> byLetter;
};

Swept swept(const Forms& listed, const Readings& readings);
void report(const Swept& swept);

} // namespace expand
