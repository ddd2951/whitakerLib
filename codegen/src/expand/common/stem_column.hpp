#pragma once

#include <algorithm>
#include <cstdint>
#include <variant>

#include "expand/common/tables.hpp"
#include "latin.hpp"
#include "types/grammar.hpp"

// INFO: A stem key is not always its column: a declared degree or numeral
//  sort re-keys a single stem, and equal first and second stems share key 0.
namespace expand::internal {

inline constexpr latin::StemKey kNoColumn{0};

[[nodiscard]] constexpr bool firstTwoEqual(const DictlineEntry& e) noexcept {
  return word::stemState(e.stem1) == word::StemState::text &&
         e.stem1 == e.stem2;
}

[[nodiscard]] constexpr latin::StemKey column(const latin::Noun&,
                                              const DictlineEntry& e,
                                              latin::StemKey key) noexcept {
  if (firstTwoEqual(e))
    return (key.value == 1 || key.value == 2) ? latin::StemKey{1} : kNoColumn;
  return key;
}
[[nodiscard]] constexpr latin::StemKey column(const latin::Adjective& d,
                                              const DictlineEntry& e,
                                              latin::StemKey key) noexcept {
  if (firstTwoEqual(e))
    return (key.value == 1 || key.value == 2) ? latin::StemKey{1} : key;
  if (d.comparison == latin::Comparison::COMP)
    return key.value == 3 ? latin::StemKey{1} : kNoColumn;
  if (d.comparison == latin::Comparison::SUPER)
    return key.value == 4 ? latin::StemKey{1} : kNoColumn;
  return key;
}
[[nodiscard]] constexpr latin::StemKey column(const latin::Adverb& d,
                                              const DictlineEntry&,
                                              latin::StemKey key) noexcept {
  if (d.comparison == latin::Comparison::COMP)
    return key.value == 2 ? latin::StemKey{1} : kNoColumn;
  if (d.comparison == latin::Comparison::SUPER)
    return key.value == 3 ? latin::StemKey{1} : kNoColumn;
  return key;
}
[[nodiscard]] constexpr latin::StemKey column(const latin::Verb&,
                                              const DictlineEntry& e,
                                              latin::StemKey key) noexcept {
  if (firstTwoEqual(e))
    return (key.value == 1 || key.value == 2) ? latin::StemKey{1} : key;
  return key;
}
[[nodiscard]] constexpr latin::StemKey column(const latin::Numeral& d,
                                              const DictlineEntry&,
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
[[nodiscard]] constexpr latin::StemKey column(const auto&, const DictlineEntry&,
                                              latin::StemKey key) noexcept {
  return key;
}

// INFO: The 1-based column serving a stem key, or kNoColumn when the entry
//  has no stem for it.
[[nodiscard]] constexpr latin::StemKey
columnForKey(const DictlineEntry& e, latin::StemKey key) noexcept {
  return std::visit([&](const auto& d) { return column(d, e, key); },
                    e.grammar);
}

[[nodiscard]] constexpr latin::Comparison
adjectiveDegree(const latin::Adjective& d, latin::StemKey key) noexcept {
  if (d.comparison == latin::Comparison::POS ||
      d.comparison == latin::Comparison::COMP ||
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

[[nodiscard]] constexpr std::uint8_t stemKeyOfColumn(const DictlineEntry& entry,
                                                     std::uint8_t column) {
  bool servesOne = false;
  bool servesTwo = false;
  std::uint8_t lowest = 10;
  for (std::uint8_t key = 0; key <= 9; ++key) {
    const latin::StemKey selected = columnForKey(entry, latin::StemKey{key});
    if (selected.value != column)
      continue;
    servesOne = servesOne || key == 1;
    servesTwo = servesTwo || key == 2;
    lowest = std::min(lowest, key);
  }
  return servesOne && servesTwo ? std::uint8_t{0} : lowest;
}

} // namespace expand::internal
