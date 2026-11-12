#pragma once

#include "source/schemes.hpp"

#include <array>
#include <meta>
#include <string>

namespace storage {

// clang-format off
template <std::meta::info file> inline std::array<std::string, [:file:] ::kLinesPerFile> lines{};
// clang-format on

} // namespace storage
