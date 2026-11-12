#pragma once

#include <cstdint>

namespace helpers {

template <typename Value> [[nodiscard]] std::uint16_t addonValue(const Value value) {
  if constexpr (requires { value.value; })
    return value.value;
  else
    return static_cast<std::uint16_t>(value);
}

} // namespace helpers
