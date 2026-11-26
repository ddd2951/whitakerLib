#pragma once

#include "facts.hpp"

#include <whitaker.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace whitaker::relationship {

inline constexpr std::size_t kMaxDictionaryFormPartLength = 24;
static_assert(kMaxWordLength <= kMaxDictionaryFormPartLength);

// INFO: DICTLINE's spelling of a missing stem, which dictionaryStemColumns() gives back.
inline constexpr std::string_view kMissingStem{"zzz"};

using DictionaryStemColumns = std::array<std::array<char, kMaxWordLength + 1>, facts::kStemColumnCount>;

[[nodiscard]] DictionaryStemColumns dictionaryStemColumns(std::uint16_t dictionary);

} // namespace whitaker::relationship
