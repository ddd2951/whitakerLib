#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "latin.hpp"

namespace source::scheme::addons {

inline constexpr std::string_view kPath{"data/source/ADDONS.LAT"};
inline constexpr std::size_t kLinesPerEntry{3};
inline constexpr std::size_t kEntriesPerFile{343};
inline constexpr std::size_t kLinesPerFile{1'200};

enum class Line : std::size_t {
  fix = 0,
  partEntry = 1,
  meaning = 2,
};

inline constexpr char kTrailingPadding{' '};
inline constexpr std::string_view kWhitespace{" "};
inline constexpr std::string_view kCommentMarker{"--"};
inline constexpr std::size_t kFixMinFields{2};
inline constexpr std::size_t kFixMaxFields{3};
inline constexpr std::size_t kGrammarMaxFields{8};

inline constexpr std::array kParts{latin::Part::N,    latin::Part::PRON,   latin::Part::V,   latin::Part::ADJ,
                                   latin::Part::NUM,  latin::Part::PACK,   latin::Part::ADV, latin::Part::PREP,
                                   latin::Part::CONJ, latin::Part::INTERJ, latin::Part::X};

inline constexpr std::size_t kPrefixFields{2};
inline constexpr std::size_t kTackonLeadFields{1};
inline constexpr std::size_t kSuffixLeadFields{3};
inline constexpr std::size_t kSuffixTailFields{1};

inline constexpr std::string_view kPrefixTag{"PREFIX"};
inline constexpr std::string_view kSuffixTag{"SUFFIX"};
inline constexpr std::string_view kTackonTag{"TACKON"};

} // namespace source::scheme::addons
