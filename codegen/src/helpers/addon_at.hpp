#pragma once

#include "word/word.hpp"

#include <cstddef>

namespace helpers {

[[nodiscard]] inline const word::AddonsEntry& addonAt(const std::size_t row) {
  return word::entry(source::AddonsIndex{row});
}

} // namespace helpers
