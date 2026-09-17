#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace source::wl_stable::uniques::scheme {

struct LineEnding {
  std::string_view symbol;
  char indicator;
};

inline constexpr std::size_t kLinesPerEntry{3};
inline constexpr std::size_t kEntriesPerFile{79};
inline constexpr std::size_t kLinesPerFile{237};
static_assert(kLinesPerFile == kEntriesPerFile * kLinesPerEntry);

inline constexpr LineEnding kLineEnding{"\n", '\n'};

static_assert(kLineEnding.symbol.size() == 1);
static_assert(kLineEnding.symbol.front() == kLineEnding.indicator);

enum class Line : std::size_t {
  word = 0,
  attributes = 1,
  meaning = 2,
};

inline constexpr char kTrailingPadding{' '};
inline constexpr std::string_view kWhitespace{" "};
inline constexpr std::size_t kMaxFields{14};

inline constexpr std::array<std::string_view, 4> kParts{"N", "PRON", "ADJ",
                                                        "V"};

inline constexpr std::size_t kNounFields{6};
inline constexpr std::size_t kPronounFields{6};
inline constexpr std::size_t kAdjectiveFields{6};
inline constexpr std::size_t kVerbFields{8};
inline constexpr std::size_t kLabelCount{5};

inline constexpr std::size_t kSensesMinLength{1};
inline constexpr char kSensesLowestByte{' '};
inline constexpr char kSensesHighestByte{'~'};

} // namespace source::wl_stable::uniques::scheme
