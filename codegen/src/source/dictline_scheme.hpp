#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "latin.hpp"

namespace source::scheme::dictline {

inline constexpr std::string_view kPath{"data/source/DICTLINE.GEN"};
inline constexpr std::size_t kLinesPerEntry{1};
inline constexpr std::size_t kEntriesPerFile{39'335};

inline constexpr std::size_t kLinesPerFile{kEntriesPerFile * kLinesPerEntry};

inline constexpr std::size_t kGrammarMaxFields{4};

inline constexpr std::array kParts{latin::Part::noun,        latin::Part::pronoun,     latin::Part::verb,
                                   latin::Part::adjective,   latin::Part::numeral,     latin::Part::packon,
                                   latin::Part::adverb,      latin::Part::preposition, latin::Part::conjunction,
                                   latin::Part::interjection};

inline constexpr char kFieldSeparator{' '};

} // namespace source::scheme::dictline
