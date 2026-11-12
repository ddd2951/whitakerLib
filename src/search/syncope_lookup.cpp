#include "search/syncope_lookup.hpp"

#include "facts.hpp"
#include "search/two_ended_bounds.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>

namespace whitaker {

namespace {

namespace rel = relationship;

// INFO: The perfect stem, WORDS' key 3, is always a verb's third column.
inline constexpr std::uint8_t kPerfectStemColumn = 2;

[[nodiscard, maybe_unused]] bool matches(const std::string_view word, const std::size_t at, const std::uint8_t rule) {
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

[[nodiscard]] std::string_view restoredLetters(const std::uint8_t rule) {
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
[[nodiscard]] bool endsInPerfect(const rel::Program program) {
  std::uint32_t cursor = program.begin;
  std::uint16_t dense = std::numeric_limits<std::uint16_t>::max();
  std::uint16_t lastEntry{};
  for (std::uint8_t i = 0; i < program.count; ++i) {
    latin::packed::Reading reading;
    std::uint16_t description;
    rel::nextReading(cursor, dense, reading, description);
    lastEntry = reading.entry;
  }

  cursor = program.begin;
  dense = std::numeric_limits<std::uint16_t>::max();
  std::uint8_t top = 0;
  bool perfect = false;
  for (std::uint8_t i = 0; i < program.count; ++i) {
    latin::packed::Reading reading;
    std::uint16_t description;
    rel::nextReading(cursor, dense, reading, description);
    if (reading.entry != lastEntry || reading.stem < top)
      continue;
    if (reading.stem > top) {
      top = reading.stem;
      perfect = false;
    }
    perfect = perfect || (reading.part == latin::Part::V && reading.stem == kPerfectStemColumn);
  }
  return perfect;
}

// INFO: Steps the perfect-suffix trie by one letter from the end; false where no perfect suffix continues.
[[nodiscard]] bool perfectStep(unsigned& node, const char letter) {
  const unsigned c = static_cast<unsigned>(static_cast<unsigned char>(facts::foldLetter(letter))) - unsigned('a');
  const auto& n = syncope::kPerfectSuffixes[node];
  if (c >= 26 || !(n.mask & (std::uint32_t{1} << c)))
    return false;
  node = n.first + static_cast<unsigned>(std::popcount(n.mask & ((std::uint32_t{1} << c) - 1)));
  return true;
}

// INFO: The word's end walked once: the node after each count of letters, and the fewest letters that end in a
//  perfect suffix.
struct PerfectTail {
  std::array<unsigned, facts::kMaxWordCharacters + 1> node{};
  unsigned alive = 0, ends = ~0u;
  explicit PerfectTail(const std::string_view word) {
    unsigned at = 0;
    for (unsigned d = 0;; ++d) {
      node[d] = at;
      alive = d;
      if (syncope::kPerfectSuffixes[at].end) {
        ends = d;
        return;
      }
      if (d == word.size() || !perfectStep(at, word[word.size() - 1 - d]))
        return;
    }
  }
  // INFO: Whether the word with `added` put in after position `at` can end in a perfect suffix.
  [[nodiscard]] bool fits(const std::string_view word, const std::size_t at, const std::string_view added) const {
    const unsigned tail = static_cast<unsigned>(word.size() - 1 - at);
    if (ends <= tail)
      return true;
    if (tail > alive)
      return false;
    unsigned n = node[tail];
    for (auto c = added.rbegin(); c != added.rend(); ++c)
      if (!perfectStep(n, *c))
        return false;
      else if (syncope::kPerfectSuffixes[n].end)
        return true;
    for (std::size_t i = at + 1; i-- > 0;)
      if (!perfectStep(n, word[i]))
        return false;
      else if (syncope::kPerfectSuffixes[n].end)
        return true;
    return false;
  }
};
static_assert(std::is_trivially_copyable_v<PerfectTail>);

} // namespace

// INFO: Five rules, in order, each trying the word's positions from the end; the
//  first full form the image holds ends the rule, and a perfect the search:
//   1  ii => ivi                    audiit    => audivit
//   2  as => avis, also es, is, os  amasti    => amavisti
//   3  ar => aver, also er, or      amarunt   => amaverunt
//   4  ier => iver                  audierunt => audiverunt
//   5  s => sis, x => xis           admisse   => admisisse
Syncope syncopeLookup(const std::string_view word) {
  if (word.size() > facts::kMaxWordCharacters)
    return {};

  // NOTE: Lowercased only, not folded u/v and i/j, as WORDS does. Fixed buffers: this runs where nothing may throw.
  std::array<char, facts::kMaxWordCharacters> lowerLetters{};
  for (std::size_t i = 0; i < word.size(); ++i)
    lowerLetters[i] = word[i] >= 'A' && word[i] <= 'Z' ? static_cast<char>(word[i] - 'A' + 'a') : word[i];
  const std::string_view lowered{lowerLetters.data(), word.size()};

  std::array<std::uint32_t, 6> at_{};
  const std::size_t n = lowered.size();
  for (std::size_t i = 0; i < n; ++i) {
    const char c = lowered[i], d = i + 1 < n ? lowered[i + 1] : '\0';
    if (d == 's') {
      if (i + 2 < n && (c == 'a' || c == 'e' || c == 'i' || c == 'o'))
        at_[2] |= 1u << i;
    } else if (d == 'r') {
      if (i > 0 && i + 2 < n && (c == 'a' || c == 'e' || c == 'o'))
        at_[3] |= 1u << i;
    } else if (d == 'i' && c == 'i')
      at_[1] |= 1u << i;
    if (c == 'i' && d == 'e' && i + 3 < n && lowered[i + 2] == 'r')
      at_[4] |= 1u << i;
    if ((c == 's' || c == 'x') && i + 2 < n)
      at_[5] |= 1u << i;
  }
  if (!(at_[1] | at_[2] | at_[3] | at_[4] | at_[5]))
    return {};
  // INFO: A position whose restored form can't end in a perfect suffix can't return. Below the lowest one that can,
  //  the rule would only end; above it, a lookup that finds something still ends the rule, so those stay.
  const PerfectTail perfect(lowered);
  for (std::uint8_t rule = 1; rule <= 5; ++rule) {
    std::uint32_t todo = at_[rule];
    while (todo && !perfect.fits(lowered, static_cast<unsigned>(std::countr_zero(todo)), restoredLetters(rule)))
      todo &= todo - 1;
    for (; todo;) {
      const std::size_t at = 31u - static_cast<unsigned>(std::countl_zero(todo));
      todo ^= 1u << at;
      const std::string_view added = restoredLetters(rule);
      if (lowered.size() + added.size() > facts::kMaxWordCharacters)
        continue;
      Syncope found{.length = static_cast<std::uint8_t>(lowered.size() + added.size()), .rule = rule};
      const auto tail = std::ranges::copy(lowered.substr(0, at + 1), found.letters.begin()).out;
      std::ranges::copy(lowered.substr(at + 1), std::ranges::copy(added, tail).out);
      const auto program = rel::lookup(found.restored());
      if (program.count == 0)
        continue;
      // NOTE: Rule 5 drops its readings before testing, so its test never fires. Rule 2 is tested like the others
      //  since the quantasvis-syncope fix in the WORDS compared against.
      if (endsInPerfect(program))
        return found;
      break;
    }
  }
  return {};
}

} // namespace whitaker
