#pragma once

#include <variant>

#include "gen/expand/wl_stable/common/tables.hpp"
#include "types/domain.hpp"
#include "types/tokenized_sources.hpp"
#include "types/types.hpp"

// NOTE: Whitaker's stem keys are not always DICTLINE columns. A declared
// adjective or adverb degree and a numeral sort re-key a single stem; equal
// first and second stems share key 0.
namespace wl_stable::expand::internal {

inline constexpr domain::StemIndex kNoColumn{0};

[[nodiscard]] constexpr bool firstTwoEqual(const DictlineEntry& e) noexcept {
  return tokenized::stemState(e.stem1) == tokenized::StemState::text &&
         e.stem1 == e.stem2;
}

[[nodiscard]] constexpr domain::StemIndex
column(const tokenized::Noun&, const DictlineEntry& e,
       domain::StemIndex key) noexcept {
  if (firstTwoEqual(e))
    return (key.value == 1 || key.value == 2) ? domain::StemIndex{1}
                                              : kNoColumn;
  return key;
}
[[nodiscard]] constexpr domain::StemIndex
column(const tokenized::Adjective& d, const DictlineEntry& e,
       domain::StemIndex key) noexcept {
  if (firstTwoEqual(e))
    return (key.value == 1 || key.value == 2) ? domain::StemIndex{1} : key;
  if (d.comparison == TypeComparison::COMP)
    return key.value == 3 ? domain::StemIndex{1} : kNoColumn;
  if (d.comparison == TypeComparison::SUPER)
    return key.value == 4 ? domain::StemIndex{1} : kNoColumn;
  return key;
}
[[nodiscard]] constexpr domain::StemIndex
column(const tokenized::Adverb& d, const DictlineEntry&,
       domain::StemIndex key) noexcept {
  if (d.comparison == TypeComparison::COMP)
    return key.value == 2 ? domain::StemIndex{1} : kNoColumn;
  if (d.comparison == TypeComparison::SUPER)
    return key.value == 3 ? domain::StemIndex{1} : kNoColumn;
  return key;
}
[[nodiscard]] constexpr domain::StemIndex
column(const tokenized::Verb&, const DictlineEntry& e,
       domain::StemIndex key) noexcept {
  if (firstTwoEqual(e))
    return (key.value == 1 || key.value == 2) ? domain::StemIndex{1} : key;
  return key;
}
[[nodiscard]] constexpr domain::StemIndex
column(const tokenized::Numeral& d, const DictlineEntry&,
       domain::StemIndex key) noexcept {
  switch (d.numeralSort) {
  case TypeNumeralSort::CARD:
    return key.value == 1 ? domain::StemIndex{1} : kNoColumn;
  case TypeNumeralSort::ORD:
    return key.value == 2 ? domain::StemIndex{1} : kNoColumn;
  case TypeNumeralSort::DIST:
    return key.value == 3 ? domain::StemIndex{1} : kNoColumn;
  case TypeNumeralSort::ADVERB:
    return key.value == 4 ? domain::StemIndex{1} : kNoColumn;
  default:
    return key;
  }
}
[[nodiscard]] constexpr domain::StemIndex
column(const auto&, const DictlineEntry&, domain::StemIndex key) noexcept {
  return key;
}

// INFO: The 1-based column serving a stem key, or kNoColumn when the entry
// has no stem for it.
[[nodiscard]] constexpr domain::StemIndex
columnForKey(const DictlineEntry& e, domain::StemIndex key) noexcept {
  return std::visit([&](const auto& d) { return column(d, e, key); },
                    e.grammar);
}

[[nodiscard]] constexpr TypeComparison
adjectiveDegree(const tokenized::Adjective& d, domain::StemIndex key) noexcept {
  if (d.comparison == TypeComparison::POS ||
      d.comparison == TypeComparison::COMP ||
      d.comparison == TypeComparison::SUPER)
    return d.comparison;
  switch (key.value) {
  case 0:
  case 1:
  case 2:
    return TypeComparison::POS;
  case 3:
    return TypeComparison::COMP;
  case 4:
    return TypeComparison::SUPER;
  default:
    return TypeComparison::X;
  }
}

} // namespace wl_stable::expand::internal
