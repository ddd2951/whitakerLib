#pragma once

#include "latin.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

// INFO: Private corpus constraints shared by the runtime and generator.

namespace facts {

// INFO: Whitaker files words under this Latin alphabet; j shares i and v shares u.
inline constexpr std::string_view kLatinLetters{"abcdefghijklmnopqrstuvwxyz"};
inline constexpr std::size_t kLetterCount{kLatinLetters.size()};

// INFO: Each dictionary entry has four stem columns.
inline constexpr std::size_t kStemColumnCount{4};

// INFO: Longest input accepted by analyze().
inline constexpr std::size_t kMaxWordCharacters{24};

// INFO: Fold one character to the letter it is filed under: upper case to lower, j to i, v to u. A character outside
//  a-z is returned unchanged and has no bucket of its own.
[[nodiscard]] constexpr char foldLetter(char c) {
  if (c >= 'A' && c <= 'Z')
    c = static_cast<char>(c - 'A' + 'a');
  if (c == 'j')
    c = 'i';
  if (c == 'v')
    c = 'u';
  return c;
}

// INFO: Positions no folded input byte can test in a letter mask. Their payload does not change letter indices.
inline constexpr std::uint32_t kFoldedLetterMask = [] {
  std::uint32_t mask = 0;
  for (unsigned c = 0; c < 256; ++c) {
    const char folded = foldLetter(static_cast<char>(static_cast<unsigned char>(c)));
    if (folded >= 'a' && folded <= 'z')
      mask |= std::uint32_t{1} << (folded - 'a');
  }
  return mask;
}();
inline constexpr std::uint32_t kFoldedAwayMask = ((std::uint32_t{1} << kLetterCount) - 1) & ~kFoldedLetterMask;

// INFO: The 0-based bucket letter, or -1 for a character with no bucket.
[[nodiscard]] constexpr int letterIndex(const char c) {
  const char folded = foldLetter(c);
  return (folded >= 'a' && folded <= 'z') ? folded - 'a' : -1;
}

// INFO: Reference List_Sweep selection.
// NOTE: Tickon, tackon and packon steps each remove at least two letters, and the library does not build otherwise;
//  prefix and suffix steps come alone, two at most.
inline constexpr std::size_t kMaxAddonSteps = kMaxWordCharacters / 2;

inline constexpr bool kOmitArchaic{true};
inline constexpr bool kOmitMedieval{false};
inline constexpr bool kOmitUncommon{true};

// NOTE: List_Sweep drops a rare reading only when one of the same group is not rare; a reading not allowed counts.
struct RarityEvidence {
  bool someNotArchaic{};
  bool someNotMedieval{};
  bool someNotUncommon{};

  constexpr void count(const latin::Age age, const latin::Age entryAge, const latin::Frequency frequency,
                       const latin::Frequency entryFrequency) {
    someNotArchaic = someNotArchaic || (!latin::archaic(age) && !latin::archaic(entryAge));
    someNotMedieval = someNotMedieval || (!latin::medieval(age) && !latin::medieval(entryAge));
    someNotUncommon =
        someNotUncommon || (!latin::uncommonInflection(frequency) && !latin::uncommonEntry(entryFrequency));
  }

  [[nodiscard]] constexpr bool drops(const latin::Age age, const latin::Frequency frequency) const {
    return (someNotArchaic && kOmitArchaic && latin::archaic(age)) ||
           (someNotMedieval && kOmitMedieval && latin::medieval(age)) ||
           (someNotUncommon && kOmitUncommon && latin::uncommonInflection(frequency));
  }
};
static_assert(std::is_trivially_copyable_v<RarityEvidence>);
// INFO: Reference Try_Tricks: Do_Medieval_Tricks.
inline constexpr bool kDoMedievalTricks{true};

} // namespace facts
