#pragma once

#include "relationship_schema.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include <type_traits>

namespace whitaker {
struct DictionaryWordStems;
}

namespace whitaker::relationship {

// INFO: The current thread's last dictionary lookup; the next lookup replaces its candidates.
struct Program {
  std::uint32_t begin{};
  std::uint8_t count{};
  bool trimmed{};
};
static_assert(std::is_trivially_copyable_v<Program>);

struct Result {
  std::uint8_t stemLength{};
  const char* stemPattern{};
  latin::Analysis grammar{};
  std::uint16_t description{};
  std::uint16_t dictionary{};
  std::uint8_t stemColumn{};
  std::uint8_t stemKey{};
};
static_assert(std::is_trivially_copyable_v<Result>);

struct Addon {
  std::uint16_t id{};
  // NOTE: Null terminated for the C lookup.
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
static_assert(std::is_trivially_copyable_v<Addon>);

// NOTE: A stem's dictionary entry with its class unpacked, for the addon search.
struct StemEntry {
  std::uint16_t dictionary{};
  std::uint8_t stemKey{};
  std::uint8_t column{};
  latin::Part part{};
  latin::VerbKind verbKind{};
  std::uint16_t rules{};
  latin::Age age{};
  latin::Frequency frequency{};
  Run rows{};
};
static_assert(std::is_trivially_copyable_v<StemEntry>);

[[nodiscard]] Program lookup(std::string_view word);
[[nodiscard]] Result next(std::uint32_t& cursor, std::uint16_t& lexeme);
void grammarReading(std::uint16_t description, latin::packed::Reading& out);
// INFO: Production output: copy the grammar bytes directly into the caller's reading.
void nextReading(std::uint32_t& cursor, std::uint16_t& lexeme, latin::packed::Reading& out, std::uint16_t& description);
// INFO: The PRON and ADJ readings List_Sweep trimmed from a spelling alone (TrimmedReadings), for Try_Tackons.
[[nodiscard]] Run trimmedReadingRange(std::string_view folded);
[[nodiscard]] TrimmedReadingRecord trimmedReading(std::uint32_t index);
[[nodiscard]] Result resultOf(const TrimmedReadingRecord& record);
[[nodiscard]] Entry entry(std::uint16_t dictionary);
[[nodiscard]] DictionaryRecord dictionary(std::uint16_t index);
[[nodiscard]] const char* dictionaryMeaning(std::uint16_t index);
[[nodiscard]] ClassRecord dictionaryClass(std::uint16_t index);
[[nodiscard]] latin::Analysis grammar(std::uint16_t description);
[[nodiscard]] ClassRowRecord classRow(std::uint32_t index);
[[nodiscard]] InflectionRecord inflection(std::uint16_t index);
[[nodiscard]] StemRecord stemRecord(std::uint32_t index);
[[nodiscard]] Run stemRange(std::string_view text);
[[nodiscard]] DictionaryWordStems stems(std::uint16_t dictionary);
[[nodiscard]] StemEntry stemEntry(std::uint32_t index);
[[nodiscard]] Run endingRun(std::string_view text);
[[nodiscard]] std::pair<latin::Age, latin::Frequency> readingLabels(std::uint16_t description,
                                                                    std::string_view spelling, std::uint8_t stemLength);
// INFO: As readingLabels, for the reading nextReading gave last from `cursor`: its rule's stored label where the
//  reading is spelled with the rule's own suffix.
[[nodiscard]] std::pair<latin::Age, latin::Frequency> readingLabels(std::uint32_t cursor, std::uint16_t description,
                                                                    std::string_view spelling, std::uint8_t stemLength);
// INFO: The inflection's age and frequency for a reading's grammar and ending; X and X for a pair the image does not
//  have.
[[nodiscard]] std::pair<latin::Age, latin::Frequency> inflectionLabels(std::uint16_t description,
                                                                       std::string_view ending);
[[nodiscard]] Addon addon(std::uint16_t id);
[[nodiscard]] std::span<const Addon> addonsOf(latin::AddonKind kind);
// INFO: Bit i is set when the i-th tackon's fix ends in that character, so a tackon loop visits only those.
[[gnu::visibility("hidden")]] extern const std::array<std::uint64_t, 256> kTackonsEndingIn;

std::size_t printStem(std::string_view spelling, std::uint8_t length, const char* pattern, char* out);

[[nodiscard]] bool loadImage(std::span<const std::byte> bytes, std::string& failure);
[[nodiscard]] const HeaderRecord& header();
[[nodiscard]] const DirectoryEntry& section(Section section);
[[nodiscard]] std::vector<std::string> spellings();

} // namespace whitaker::relationship
