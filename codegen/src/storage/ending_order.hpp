#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace storage {

using EndingOrder = std::vector<std::pair<std::string, std::uint32_t>>;

inline EndingOrder endingOrder{};

} // namespace storage
