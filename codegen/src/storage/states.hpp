#pragma once

#include <compare>
#include <cstdint>
#include <vector>

namespace storage {

struct State {
  std::uint32_t mask{};
  bool stem{};
  std::vector<unsigned> kids{};
  auto operator<=>(const State&) const = default;
};

inline std::vector<State> states{};

} // namespace storage
