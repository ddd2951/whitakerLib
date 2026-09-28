#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "facts.hpp"
#include "expand/common/form.hpp"
#include "expand/facts/sorted.hpp"
#include "types/grammar.hpp"

namespace expand {

struct AdverbCorrections {
  struct Correction {
    std::size_t after;
    Form form;
  };
  std::array<std::vector<Correction>, facts::kLetterCount> byLetter;
};

AdverbCorrections adverbCorrections(const Sorted& sorted);

} // namespace expand
