#pragma once

#include "source/schemes.hpp"

#include <array>
#include <cstddef>
#include <meta>

namespace storage {

// clang-format off
template <std::meta::info file>
inline std::array<std::size_t, [:file:] ::kEntriesPerFile * [:file:] ::kLinesPerEntry> records{};
// clang-format on

} // namespace storage
