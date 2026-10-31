#pragma once

#include "latin.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

// Private corpus constraints shared by the runtime and generator.

namespace facts {

// Whitaker files words under this Latin alphabet; j shares i and v shares u.
inline constexpr std::string_view kLatinLetters{"abcdefghijklmnopqrstuvwxyz"};
inline constexpr std::size_t kLetterCount{kLatinLetters.size()};

// Each dictionary entry has four stem columns.
inline constexpr std::size_t kStemColumnCount{4};

// Longest input accepted by analyze().
inline constexpr std::size_t kMaxWordCharacters{24};

// Fold one character to the letter it is filed under: upper case to lower, j to i, v to u. A character outside a-z is
// returned unchanged and has no bucket of its own.
[[nodiscard]] constexpr char foldLetter(char c) noexcept {
  if (c >= 'A' && c <= 'Z')
    c = static_cast<char>(c - 'A' + 'a');
  if (c == 'j')
    c = 'i';
  if (c == 'v')
    c = 'u';
  return c;
}

// The 0-based bucket letter, or -1 for a character with no bucket.
[[nodiscard]] constexpr int letterIndex(char c) noexcept {
  const char folded = foldLetter(c);
  return (folded >= 'a' && folded <= 'z') ? folded - 'a' : -1;
}

// Reference List_Sweep selection.
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

  constexpr void count(latin::Age age, latin::Age entryAge, latin::Frequency frequency,
                       latin::Frequency entryFrequency) noexcept {
    someNotArchaic = someNotArchaic || (!latin::archaic(age) && !latin::archaic(entryAge));
    someNotMedieval = someNotMedieval || (!latin::medieval(age) && !latin::medieval(entryAge));
    someNotUncommon =
        someNotUncommon || (!latin::uncommonInflection(frequency) && !latin::uncommonEntry(entryFrequency));
  }

  [[nodiscard]] constexpr bool drops(latin::Age age, latin::Frequency frequency) const noexcept {
    return (someNotArchaic && kOmitArchaic && latin::archaic(age)) ||
           (someNotMedieval && kOmitMedieval && latin::medieval(age)) ||
           (someNotUncommon && kOmitUncommon && latin::uncommonInflection(frequency));
  }
};
// Reference Try_Tricks: Do_Medieval_Tricks.
inline constexpr bool kDoMedievalTricks{true};

} // namespace facts
