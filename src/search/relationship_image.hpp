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
};

struct Result {
  std::uint8_t stemLength{};
  const char* stemPattern{};
  latin::Analysis grammar{};
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

[[nodiscard]] Program lookup(std::string_view word) noexcept;
[[nodiscard]] Result next(std::uint32_t& cursor, std::uint16_t& lexeme) noexcept;
[[nodiscard]] Entry entry(std::uint16_t dictionary) noexcept;
[[nodiscard]] DictionaryRecord dictionary(std::uint16_t index) noexcept;
[[nodiscard]] const char* dictionaryMeaning(std::uint16_t index) noexcept;
[[nodiscard]] ClassRecord dictionaryClass(std::uint16_t index) noexcept;
[[nodiscard]] latin::Analysis grammar(std::uint16_t description) noexcept;
[[nodiscard]] FallbackRowRecord fallbackRow(std::uint32_t index) noexcept;
[[nodiscard]] InflectionRecord inflection(std::uint16_t index) noexcept;
[[nodiscard]] FallbackStemRecord fallbackStem(std::uint32_t index) noexcept;
[[nodiscard]] Run fallbackStemRange(std::string_view text) noexcept;
[[nodiscard]] Run endingRun(std::string_view text) noexcept;
[[nodiscard]] Addon addon(std::uint16_t id) noexcept;
[[nodiscard]] std::span<const Addon> addonsOf(latin::AddonKind kind) noexcept;

std::size_t printStem(std::string_view spelling, std::uint8_t length, const char* pattern, char* out) noexcept;

[[nodiscard]] bool loadImage(std::span<const std::byte> bytes, std::string& failure);
[[nodiscard]] const HeaderRecord& header() noexcept;
[[nodiscard]] const DirectoryEntry& section(Section section) noexcept;
[[nodiscard]] std::vector<std::string> spellings();

} // namespace whitaker::relationship
