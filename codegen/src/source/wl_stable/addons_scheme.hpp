#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace source::wl_stable::addons::scheme {

struct LineEnding {
  std::string_view symbol;
  char indicator;
};

inline constexpr std::size_t kLinesPerEntry{3};
inline constexpr std::size_t kEntriesPerFile{343};
inline constexpr std::size_t kLinesPerFile{1'200};
static_assert(kLinesPerFile > kEntriesPerFile * kLinesPerEntry);

inline constexpr LineEnding kLineEnding{"\n", '\n'};

static_assert(kLineEnding.symbol.size() == 1);
static_assert(kLineEnding.symbol.front() == kLineEnding.indicator);

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

inline constexpr std::array<std::string_view, 11> kParts{
    "N",   "PRON", "V",    "ADJ",    "NUM", "PACK",
    "ADV", "PREP", "CONJ", "INTERJ", "X"};

inline constexpr std::size_t kAnyFields{0};
inline constexpr std::size_t kPrefixFields{2};
inline constexpr std::size_t kTackonLeadFields{1};
inline constexpr std::size_t kSuffixLeadFields{3};
inline constexpr std::size_t kSuffixTailFields{1};

inline constexpr std::string_view kPrefixTag{"PREFIX"};
inline constexpr std::string_view kSuffixTag{"SUFFIX"};
inline constexpr std::string_view kTackonTag{"TACKON"};
inline constexpr std::size_t kMeaningMinLength{1};
inline constexpr char kMeaningLowestByte{' '};
inline constexpr char kMeaningHighestByte{'~'};

} // namespace source::wl_stable::addons::scheme
