#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

#include "latin.hpp"
#include "types/grammar.hpp"
#include "word/word.hpp"

// INFO: A stem key is not always its column: a declared degree or numeral sort re-keys a single stem, and equal first
//  and second stems share key 0.
namespace expand::internal {

inline constexpr latin::StemKey kNoColumn{0};

[[nodiscard]] constexpr bool firstTwoEqual(const word::DictlineEntry& e) noexcept {
  return e.stem1 && !e.stem1->empty() && e.stem1 == e.stem2;
}

[[nodiscard]] constexpr latin::StemKey column(const latin::Noun&, const word::DictlineEntry& e,
                                              latin::StemKey key) noexcept {
  if (firstTwoEqual(e))
    return (key.value == 1 || key.value == 2) ? latin::StemKey{1} : kNoColumn;
  return key;
}
[[nodiscard]] constexpr latin::StemKey column(const latin::Adjective& d, const word::DictlineEntry& e,
                                              latin::StemKey key) noexcept {
  if (firstTwoEqual(e))
    return (key.value == 1 || key.value == 2) ? latin::StemKey{1} : key;
  if (d.comparison == latin::Comparison::COMP)
    return key.value == 3 ? latin::StemKey{1} : kNoColumn;
  if (d.comparison == latin::Comparison::SUPER)
    return key.value == 4 ? latin::StemKey{1} : kNoColumn;
  return key;
}
[[nodiscard]] constexpr latin::StemKey column(const latin::Adverb& d, const word::DictlineEntry&,
                                              latin::StemKey key) noexcept {
  if (d.comparison == latin::Comparison::COMP)
    return key.value == 2 ? latin::StemKey{1} : kNoColumn;
  if (d.comparison == latin::Comparison::SUPER)
    return key.value == 3 ? latin::StemKey{1} : kNoColumn;
  return key;
}
[[nodiscard]] constexpr latin::StemKey column(const latin::Verb&, const word::DictlineEntry& e,
                                              latin::StemKey key) noexcept {
  if (firstTwoEqual(e))
    return (key.value == 1 || key.value == 2) ? latin::StemKey{1} : key;
  return key;
}
[[nodiscard]] constexpr latin::StemKey column(const latin::Numeral& d, const word::DictlineEntry&,
                                              latin::StemKey key) noexcept {
  switch (d.numeralSort) {
  case latin::NumeralSort::CARD:
    return key.value == 1 ? latin::StemKey{1} : kNoColumn;
  case latin::NumeralSort::ORD:
    return key.value == 2 ? latin::StemKey{1} : kNoColumn;
  case latin::NumeralSort::DIST:
    return key.value == 3 ? latin::StemKey{1} : kNoColumn;
  case latin::NumeralSort::ADVERB:
    return key.value == 4 ? latin::StemKey{1} : kNoColumn;
  default:
    return key;
  }
}
[[nodiscard]] constexpr latin::StemKey column(const auto&, const word::DictlineEntry&, latin::StemKey key) noexcept {
  return key;
}

// INFO: The 1-based column serving a stem key, or kNoColumn when the entry has no stem for it.
[[nodiscard]] constexpr latin::StemKey columnForKey(const word::DictlineEntry& e, latin::StemKey key) noexcept {
  return std::visit([&](const auto& d) { return column(d, e, key); }, e.grammar);
}

[[nodiscard]] constexpr latin::Comparison adjectiveDegree(const latin::Adjective& d, latin::StemKey key) noexcept {
  if (d.comparison == latin::Comparison::POS || d.comparison == latin::Comparison::COMP ||
      d.comparison == latin::Comparison::SUPER)
    return d.comparison;
  switch (key.value) {
  case 0:
  case 1:
  case 2:
    return latin::Comparison::POS;
  case 3:
    return latin::Comparison::COMP;
  case 4:
    return latin::Comparison::SUPER;
  default:
    return latin::Comparison::X;
  }
}

[[nodiscard]] constexpr std::optional<latin::StemKey> stemKeyOfColumn(const word::DictlineEntry& entry,
                                                                      std::uint8_t column) {
  bool servesOne = false;
  bool servesTwo = false;
  std::optional<latin::StemKey> lowest;
  for (latin::StemKey key{0}; latin::isValid(key); ++key.value) {
    if (columnForKey(entry, key).value != column)
      continue;
    servesOne = servesOne || key.value == 1;
    servesTwo = servesTwo || key.value == 2;
    if (!lowest)
      lowest = key;
  }
  if (servesOne && servesTwo)
    return latin::StemKey{0};
  return lowest;
}

} // namespace expand::internal

namespace expand {

[[nodiscard]] constexpr word::Stem stemAt(const word::DictlineEntry& entry, latin::StemKey column) {
  constexpr std::array stems{&word::DictlineEntry::stem1, &word::DictlineEntry::stem2, &word::DictlineEntry::stem3,
                             &word::DictlineEntry::stem4};
  const std::size_t at = column.value;
  if (at < 1 || at > stems.size())
    return std::nullopt;
  return entry.*stems[at - 1];
}

} // namespace expand
