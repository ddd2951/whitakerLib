#pragma once

#include <string_view>

#include "facts.hpp"

namespace expand::config {

// NOTE: The letters a generation expands. Every letter of the alphabet
// unless an experiment narrows it.
inline constexpr std::string_view kLetters{facts::kLatinLetters};

} // namespace expand::config
