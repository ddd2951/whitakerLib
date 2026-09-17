#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace source::wl_stable::dictline::scheme {

struct LineEnding {
  std::string_view symbol;
  char indicator;
};

inline constexpr std::size_t kLinesPerEntry{1};
inline constexpr std::size_t kEntriesPerFile{39'335};

inline constexpr std::size_t kLinesPerFile{39'335};
static_assert(kLinesPerFile == kEntriesPerFile * kLinesPerEntry);

inline constexpr LineEnding kLineEnding{"\r\n", '\r'};

static_assert(kLineEnding.symbol.size() == 2);
static_assert(kLineEnding.symbol.front() == kLineEnding.indicator);

inline constexpr std::size_t kStemCount{4};
inline constexpr std::size_t kStemWidth{19};
inline constexpr std::size_t kPartWidth{7};
inline constexpr std::size_t kGrammarWidth{16};
inline constexpr std::size_t kGrammarMaxFields{4};

inline constexpr std::array<std::string_view, 10> kParts{
    "N",   "PRON", "V",    "ADJ",  "NUM",
    "PACK", "ADV",  "PREP", "CONJ", "INTERJ"};

inline constexpr std::size_t kNounFields{4};
inline constexpr std::size_t kPronounFields{3};
inline constexpr std::size_t kVerbFields{3};
inline constexpr std::size_t kAdjectiveFields{3};
inline constexpr std::size_t kNumeralFields{4};
inline constexpr std::size_t kPackonFields{3};
inline constexpr std::size_t kAdverbFields{1};
inline constexpr std::size_t kPrepositionFields{1};
inline constexpr std::size_t kConjunctionFields{0};
inline constexpr std::size_t kInterjectionFields{0};
inline constexpr std::size_t kLabelCount{5};

inline constexpr std::size_t kLabelStride{2};
inline constexpr std::size_t kLabelsWidth{kLabelCount * kLabelStride + 1};
static_assert(kLabelsWidth == 11);

inline constexpr std::size_t kAgeAt{1};
inline constexpr std::size_t kAreaAt{kAgeAt + kLabelStride};
inline constexpr std::size_t kGeographyAt{kAreaAt + kLabelStride};
inline constexpr std::size_t kFrequencyAt{kGeographyAt + kLabelStride};
inline constexpr std::size_t kSourceAt{kFrequencyAt + kLabelStride};
static_assert(kSourceAt + 1 < kLabelsWidth);

inline constexpr char kFieldSeparator{' '};

inline constexpr std::size_t kStemAt{0};
inline constexpr std::size_t kPartAt{kStemAt + kStemCount * kStemWidth};
inline constexpr std::size_t kGrammarAt{kPartAt + kPartWidth};
inline constexpr std::size_t kLabelsAt{kGrammarAt + kGrammarWidth};
inline constexpr std::size_t kSensesAt{kLabelsAt + kLabelsWidth};
static_assert(kSensesAt == 110);

inline constexpr std::size_t kSensesMinLength{1};
inline constexpr char kSensesLowestByte{' '};
inline constexpr char kSensesHighestByte{'~'};

} // namespace source::wl_stable::dictline::scheme
