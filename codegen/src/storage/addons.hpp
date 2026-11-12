#pragma once

#include "word/word.hpp"

#include <array>

namespace storage {

inline std::array<word::AddonsEntry, source::scheme::addons::kEntriesPerFile> addons{};

} // namespace storage
