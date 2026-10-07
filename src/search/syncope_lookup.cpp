#include "search/syncope_lookup.hpp"

#include "facts.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace whitaker {

namespace {

namespace rel = relationship;

// The perfect stem, WORDS' key 3, is always a verb's third column.
inline constexpr std::uint8_t kPerfectStemColumn = 2;

[[nodiscard]] bool matches(std::string_view word, std::size_t at, std::uint8_t rule) noexcept {
  switch (rule) {
  case 1:
    return word.substr(at, 2) == "ii";
  case 2:
    return at + 2 < word.size() && (word[at] == 'a' || word[at] == 'e' || word[at] == 'i' || word[at] == 'o') &&
           word[at + 1] == 's';
  case 3:
    return at > 0 && at + 2 < word.size() && (word[at] == 'a' || word[at] == 'e' || word[at] == 'o') &&
           word[at + 1] == 'r';
  case 4:
    return at + 3 < word.size() && word.substr(at, 3) == "ier";
  case 5:
    return at + 2 < word.size() && (word[at] == 's' || word[at] == 'x');
  }
  return false;
}

[[nodiscard]] std::string_view restoredLetters(std::uint8_t rule) noexcept {
  switch (rule) {
  case 1:
  case 4:
    return "v";
  case 2:
    return "vi";
  case 3:
    return "ve";
  case 5:
    return "is";
  }
  return {};
}

// NOTE: WORDS asks whether the last reading it appended is a perfect; the image keeps no such order, so this asks of
//  the last entry's highest column instead.
[[nodiscard]] bool endsInPerfect(rel::Program program) noexcept {
  std::uint32_t cursor = program.begin;
  std::uint16_t dense = std::numeric_limits<std::uint16_t>::max();
  std::uint16_t lastEntry{};
  for (std::uint8_t i = 0; i < program.count; ++i)
    lastEntry = rel::next(cursor, dense).dictionary;

  cursor = program.begin;
  dense = std::numeric_limits<std::uint16_t>::max();
  std::uint8_t top = 0;
  bool perfect = false;
  for (std::uint8_t i = 0; i < program.count; ++i) {
    const rel::Result reading = rel::next(cursor, dense);
    if (reading.dictionary != lastEntry || reading.stemColumn < top)
      continue;
    if (reading.stemColumn > top) {
      top = reading.stemColumn;
      perfect = false;
    }
    perfect = perfect || (reading.grammar.part == latin::Part::V && reading.stemColumn == kPerfectStemColumn);
  }
  return perfect;
}

} // namespace

// Five rules, in order, each trying the word's positions from the end; the
// first full form the image holds ends the rule, and a perfect the search:
//   1  ii => ivi                    audiit    => audivit
//   2  as => avis, also es, is, os  amasti    => amavisti
//   3  ar => aver, also er, or      amarunt   => amaverunt
//   4  ier => iver                  audierunt => audiverunt
//   5  s => sis, x => xis           admisse   => admisisse
std::string syncopeLookup(std::string_view word) {
  if (word.size() > facts::kMaxWordCharacters)
    return {};

  // NOTE: Lowercased only, not folded u/v and i/j, as WORDS does.
  std::string lowered{word};
  for (char& c : lowered)
    if (c >= 'A' && c <= 'Z')
      c = static_cast<char>(c - 'A' + 'a');

  for (std::uint8_t rule = 1; rule <= 5; ++rule) {
    for (std::size_t at = lowered.size(); at-- > 0;) {
      if (!matches(lowered, at, rule))
        continue;
      const std::string_view added = restoredLetters(rule);
      if (lowered.size() + added.size() > facts::kMaxWordCharacters)
        continue;
      std::string restored{lowered};
      restored.insert(at + 1, added);
      const auto program = rel::lookup(restored);
      if (program.count == 0)
        continue;
      // NOTE: WORDS returns rule 2 even when this test fails, and rule 5 drops its readings before testing, so its test
      //  never fires.
      if (rule == 2 || endsInPerfect(program))
        return restored;
      break;
    }
  }
  return {};
}

} // namespace whitaker
