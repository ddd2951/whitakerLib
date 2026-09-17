#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace source::wl_stable::inflects::scheme {

struct LineEnding {
  std::string_view symbol;
  char indicator;
};

inline constexpr std::size_t kLinesPerEntry{1};
inline constexpr std::size_t kEntriesPerFile{1'797};
inline constexpr std::size_t kLinesPerFile{3'228};
static_assert(kLinesPerFile > kEntriesPerFile * kLinesPerEntry);

inline constexpr LineEnding kLineEnding{"\r\n", '\r'};

static_assert(kLineEnding.symbol.size() == 2);
static_assert(kLineEnding.symbol.front() == kLineEnding.indicator);

inline constexpr std::string_view kWhitespace{" \t"};
inline constexpr std::string_view kCommentMarker{"--"};

inline constexpr char kFieldSeparator{' '};
inline constexpr std::size_t kMaxFields{14};

inline constexpr std::array<std::string_view, 11> kParts{
    "N",      "PRON", "ADJ",  "NUM",  "V",     "VPAR",
    "SUPINE", "ADV",  "PREP", "CONJ", "INTERJ"};

inline constexpr std::size_t kNounFields{5};
inline constexpr std::size_t kPronounFields{5};
inline constexpr std::size_t kAdjectiveFields{6};
inline constexpr std::size_t kNumeralFields{6};
inline constexpr std::size_t kVerbFields{7};
inline constexpr std::size_t kParticipleFields{8};
inline constexpr std::size_t kSupineFields{5};
inline constexpr std::size_t kAdverbFields{1};
inline constexpr std::size_t kPrepositionFields{1};
inline constexpr std::size_t kConjunctionFields{0};
inline constexpr std::size_t kInterjectionFields{0};
inline constexpr std::size_t kEndingFields{2};
inline constexpr std::size_t kLabelCount{2};

} // namespace source::wl_stable::inflects::scheme
