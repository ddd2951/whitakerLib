#pragma once

#include <cstdint>
#include <type_traits>
#include <vector>

namespace storage {

struct Cell {
  unsigned first{}, count{};
  std::uint64_t keep{};
};
static_assert(std::is_trivially_copyable_v<Cell>);

inline std::vector<Cell> cells{{0, 0, 0}};

} // namespace storage
