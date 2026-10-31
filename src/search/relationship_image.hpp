#pragma once

#include "relationship_schema.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace whitaker::relationship {

struct Program {
  std::uint32_t begin{};
  std::uint8_t count{};
  bool trimmed{};
};

struct Result {
  std::uint8_t stemLength{};
  const char* stemPattern{};
  latin::Analysis grammar{};
  std::uint16_t description{};
  std::uint16_t dictionary{};
  std::uint8_t stemColumn{};
};

struct Addon {
  std::uint16_t id{};
  // Null terminated for the C lookup.
  std::string_view fix{};
  const char* meaning{};
  std::array<std::uint16_t, 4> target{};
  latin::AddonKind kind{};
  latin::Part root{};
  latin::Part targetPart{};
  std::uint8_t rootStemKey{};
  std::uint8_t targetStemKey{};
  char connectingLetter{};
  std::uint8_t targetRules{};
  char firstLetterAsSpelled{};
  Run rows{};
};

// NOTE: A stem's dictionary entry with its class unpacked, for the addon search.
struct StemEntry {
  std::uint16_t dictionary{};
  std::uint8_t stemKey{};
  latin::Part part{};
  latin::VerbKind verbKind{};
  std::uint16_t rules{};
  latin::Age age{};
  latin::Frequency frequency{};
  Run rows{};
};

[[nodiscard]] Program lookup(std::string_view word) noexcept;
[[nodiscard]] Result next(std::uint32_t& cursor, std::uint16_t& lexeme) noexcept;
// The PRON and ADJ readings List_Sweep trimmed from a spelling alone (TrimmedReadings), for Try_Tackons.
[[nodiscard]] Run trimmedReadingRange(std::string_view folded) noexcept;
[[nodiscard]] TrimmedReadingRecord trimmedReading(std::uint32_t index) noexcept;
[[nodiscard]] Result resultOf(const TrimmedReadingRecord& record) noexcept;
[[nodiscard]] Entry entry(std::uint16_t dictionary) noexcept;
[[nodiscard]] DictionaryRecord dictionary(std::uint16_t index) noexcept;
[[nodiscard]] const char* dictionaryMeaning(std::uint16_t index) noexcept;
[[nodiscard]] ClassRecord dictionaryClass(std::uint16_t index) noexcept;
[[nodiscard]] latin::Analysis grammar(std::uint16_t description) noexcept;
[[nodiscard]] ClassRowRecord classRow(std::uint32_t index) noexcept;
[[nodiscard]] InflectionRecord inflection(std::uint16_t index) noexcept;
[[nodiscard]] StemRecord stemRecord(std::uint32_t index) noexcept;
[[nodiscard]] Run stemRange(std::string_view text) noexcept;
[[nodiscard]] StemEntry stemEntry(std::uint32_t index) noexcept;
[[nodiscard]] Run endingRun(std::string_view text) noexcept;
// The inflection's age and frequency for a reading's grammar and ending; X and X for a pair the image does not have.
[[nodiscard]] std::pair<latin::Age, latin::Frequency> inflectionLabels(std::uint16_t description,
                                                                       std::string_view ending) noexcept;
[[nodiscard]] Addon addon(std::uint16_t id) noexcept;
[[nodiscard]] std::span<const Addon> addonsOf(latin::AddonKind kind) noexcept;

std::size_t printStem(std::string_view spelling, std::uint8_t length, const char* pattern, char* out) noexcept;

[[nodiscard]] bool loadImage(std::span<const std::byte> bytes, std::string& failure);
[[nodiscard]] const HeaderRecord& header() noexcept;
[[nodiscard]] const DirectoryEntry& section(Section section) noexcept;
[[nodiscard]] std::vector<std::string> spellings();

} // namespace whitaker::relationship
