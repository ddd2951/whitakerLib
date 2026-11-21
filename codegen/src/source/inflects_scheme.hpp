#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "latin.hpp"

namespace source::scheme::inflects {

inline constexpr std::string_view kPath{"data/source/INFLECTS.LAT"};
inline constexpr std::size_t kLinesPerEntry{1};
inline constexpr std::size_t kEntriesPerFile{1'797};
inline constexpr std::size_t kLinesPerFile{3'228};

inline constexpr std::string_view kWhitespace{" \t"};
inline constexpr std::string_view kCommentMarker{"--"};

inline constexpr std::size_t kMaxFields{14};

inline constexpr std::array kParts{latin::Part::noun,        latin::Part::pronoun,     latin::Part::adjective,
                                   latin::Part::numeral,     latin::Part::verb,        latin::Part::participle,
                                   latin::Part::supine,      latin::Part::adverb,      latin::Part::preposition,
                                   latin::Part::conjunction, latin::Part::interjection};

} // namespace source::scheme::inflects
