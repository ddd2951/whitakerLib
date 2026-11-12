#pragma once

#include <string_view>
#include <type_traits>

#include "expand/common/form.hpp"
#include "latin.hpp"
#include "types/grammar.hpp"

namespace expand {

struct Reading {
  std::string_view stem{};
  latin::StemKey key{};
  latin::Entry entry{};
  latin::Inflection inflection{};
  latin::Age entryAge{};
  latin::Frequency entryFrequency{};
  latin::Age inflectionAge{};
  latin::Frequency inflectionFrequency{};
  std::string_view senses{};
  bool unique{};
};
static_assert(std::is_trivially_copyable_v<Reading>);

[[nodiscard]] Reading readingOf(const Origin& origin);

} // namespace expand
