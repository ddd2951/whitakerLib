#pragma once

#include "facts.hpp"
#include "latin.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace whitaker::relationship {

enum class Section : std::uint32_t {
  States,
  Transitions,
  Lexemes,
  Paradigms,
  Targets,
  Instructions,
  Dictionaries,
  Addons,
  Classes,
  FallbackRows,
  Inflections,
  Endings,
  FallbackStems,
  Strings,
  Grammars,
  Entries,
  count,
};

inline constexpr std::array<char, 8> kMagic{'W', 'H', 'I', 'T',
                                            'R', 'E', 'L', '\0'};
inline constexpr std::uint32_t kVersion = 5;
inline constexpr std::uint32_t kSectionCount =
    static_cast<std::uint32_t>(Section::count);
inline constexpr std::size_t kHeaderBytes = 48;
inline constexpr std::size_t kDirectoryBytes = 24;
inline constexpr std::size_t kStateBytes = 12;
inline constexpr std::size_t kTransitionBytes = 8;
inline constexpr std::size_t kLexemeBytes = 8;
inline constexpr std::size_t kParadigmBytes = 8;
inline constexpr std::size_t kTargetBytes = 2;
inline constexpr std::size_t kDictionaryBytes =
    (facts::kStemColumnCount + 2) * sizeof(std::uint32_t);
// The dictionary record's gate bits, read by the fallback.
inline constexpr std::uint16_t kGateAbbreviation = 1u << 0;
inline constexpr std::uint16_t kGateInterjOrConj = 1u << 1;
inline constexpr std::uint16_t kGateConjThreeOne = 1u << 2;
// Which packon a PACK class answers to: a one-based ordinal into the addon
// table's packon records, zero for any other class.
inline constexpr std::uint16_t kGatePackonShift = 8;
inline constexpr std::uint16_t kGatePackonMask = 0xfu;
// The verb kind rides on the entry, not the class: two verbs of one
// conjugation share a class and can still differ here.
inline constexpr std::uint16_t kClassIdMask = 0x0fffu;
inline constexpr std::uint16_t kClassVerbKindShift = 12;
inline constexpr std::uint16_t kClassVerbKindMask = 0xfu;
// One description's grammar, by description id, one byte a field.
inline constexpr std::size_t kGrammarBytes = 12;
inline constexpr std::size_t kEntryBytes = 12;
// One ADDONS.LAT record. The fix is stored folded; `firstRaw` is its first
// character as spelled, which Apply_Prefix compares unfolded.
inline constexpr std::size_t kAddonBytes = 32;
// One (inflection, description) pair a grammar licenses. A class and a
// suffix's target each name a contiguous run of these.
inline constexpr std::size_t kFallbackRowBytes = 4;
// One distinct dictionary grammar, with its run of licensed rows. The part
// of speech is on the fallback stem, not here.
inline constexpr std::size_t kClassBytes = 8;
// One INFLECTS.LAT row, in ending order.
inline constexpr std::size_t kInflectionBytes = 8;
// One distinct ending, naming its run of inflection rows.
inline constexpr std::size_t kEndingBytes = 8;
// One folded dictionary stem for the fallback; `key` is Whitaker's stem key,
// not a source column.
inline constexpr std::size_t kFallbackStemBytes = 8;
inline constexpr std::uint32_t kLetterMask = (1u << facts::kLetterCount) - 1;
inline constexpr std::uint8_t kLexemeChange = 0x80u;
inline constexpr std::uint8_t kRelationshipMask = 0x7fu;
inline constexpr std::size_t kMaximumResultsPerSpelling = 127;
inline constexpr std::size_t kMaximumTargetsPerParadigm = kRelationshipMask;
inline constexpr std::uint32_t kMaximumWordLength = 24;

struct DirectoryEntry {
  std::uint64_t offset{};
  std::uint64_t bytes{};
  std::uint64_t count{};
};

struct Entry {
  latin::Part part{};
  std::uint8_t which{};
  std::uint8_t variant{};
  latin::Gender gender{};
  std::uint8_t kind{};
  latin::Comparison comparison{};
  latin::NumeralSort numeralSort{};
  latin::Area area{};
  latin::Geography geography{};
  latin::Source source{};
  latin::NumeralValue numeralValue{};
  friend constexpr bool operator==(Entry, Entry) = default;
};

[[nodiscard]] constexpr std::uint16_t read16(const std::byte* p) noexcept {
  return static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(p[0])) |
         static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(p[1]) << 8);
}

[[nodiscard]] constexpr std::uint32_t read32(const std::byte* p) noexcept {
  return static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[0])) |
         (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[1]))
          << 8) |
         (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[2]))
          << 16) |
         (static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(p[3]))
          << 24);
}

// A state's edge count is the population of its letter mask.
[[nodiscard]] constexpr std::uint32_t popcount32(std::uint32_t mask) noexcept {
  return static_cast<std::uint32_t>(std::popcount(mask));
}

[[nodiscard]] constexpr std::uint64_t read64(const std::byte* p) noexcept {
  return read32(p) | (static_cast<std::uint64_t>(read32(p + 4)) << 32);
}

} // namespace whitaker::relationship
