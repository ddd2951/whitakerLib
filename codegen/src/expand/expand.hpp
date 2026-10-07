#pragma once

#include "expand/common/form.hpp"
#include "expand/common/reading.hpp"
#include "expand/facts/swept.hpp"

#include <cstddef>
#include <string_view>

namespace expand {

struct ResultIndex {
  std::size_t value{};
  friend constexpr bool operator==(ResultIndex, ResultIndex) = default;
};

void init();

[[nodiscard]] std::size_t resultCount();

[[nodiscard]] std::string_view spelling(ResultIndex result);
[[nodiscard]] Origin origin(ResultIndex result);
[[nodiscard]] Reading reading(ResultIndex result);
[[nodiscard]] Fate fate(ResultIndex result);

} // namespace expand
