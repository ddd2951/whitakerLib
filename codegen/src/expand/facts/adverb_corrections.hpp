#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <vector>
#include <type_traits>

#include "facts.hpp"
#include "expand/common/form.hpp"
#include "expand/common/reading.hpp"
#include "expand/facts/sorted.hpp"
#include "types/grammar.hpp"

namespace expand {

struct AdverbCorrections {
  struct Correction {
    std::size_t after{};
    Form form{};
  };
  static_assert(std::is_trivially_copyable_v<Correction>);
  std::array<std::vector<Correction>, facts::kLetterCount> byLetter{};
};

AdverbCorrections adverbCorrections(const Sorted& sorted);

} // namespace expand
