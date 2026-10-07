#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "latin.hpp"

namespace source::scheme::uniques {

inline constexpr std::string_view kPath{"data/source/UNIQUES.LAT"};
inline constexpr std::size_t kLinesPerEntry{3};
inline constexpr std::size_t kEntriesPerFile{79};
inline constexpr std::size_t kLinesPerFile{kEntriesPerFile * kLinesPerEntry};

enum class Line : std::size_t {
  word = 0,
  attributes = 1,
  meaning = 2,
};

inline constexpr char kTrailingPadding{' '};
inline constexpr std::string_view kWhitespace{" "};

inline constexpr std::array kParts{latin::Part::N, latin::Part::PRON, latin::Part::ADJ, latin::Part::V};

inline constexpr std::size_t kLabelCount{5};

} // namespace source::scheme::uniques
