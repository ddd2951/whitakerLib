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

inline constexpr std::array kParts{latin::Part::N,    latin::Part::PRON, latin::Part::ADJ,    latin::Part::NUM,
                                   latin::Part::V,    latin::Part::VPAR, latin::Part::SUPINE, latin::Part::ADV,
                                   latin::Part::PREP, latin::Part::CONJ, latin::Part::INTERJ};

} // namespace source::scheme::inflects
