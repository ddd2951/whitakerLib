#pragma once

#include "word/word.hpp"

#include <array>

namespace storage {

inline std::array<word::UniquesEntry, source::scheme::uniques::kEntriesPerFile> uniques{};

} // namespace storage
