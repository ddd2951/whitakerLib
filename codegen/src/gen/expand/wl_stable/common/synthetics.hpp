#pragma once

#include <cstdint>
#include <meta>

#include "gen/expand/wl_stable/common/form.hpp"
#include "gen/expand/wl_stable/common/tables.hpp"

namespace wl_stable::expand {

struct Synthetics {
  DictlineEntry esse;
  InflectsEntry adverbPositive;
  InflectsEntry adverbSuperlative;
};

namespace internal {
namespace broken {
void rowOfNamesAMemberOfSynthetics();
void rowOfNamesARowOfAnotherTable();
void rowOfNamesATableWithNoSyntheticRows();
}

template <typename Row> consteval Row rowOf(std::meta::info member) {
  constexpr auto row = std::meta::dealias(^^Row);
  constexpr bool dictline = row == std::meta::dealias(^^DictlineRow);
  constexpr bool inflects = row == std::meta::dealias(^^InflectsRow);
  if (!dictline && !inflects)
    broken::rowOfNamesATableWithNoSyntheticRows();
  if (std::meta::type_of(member) !=
      (dictline ? ^^DictlineEntry : ^^InflectsEntry))
    broken::rowOfNamesARowOfAnotherTable();
  std::uint32_t seen = 0;
  bool found = false;
  for (const auto candidate : std::meta::nonstatic_data_members_of(
           ^^Synthetics, std::meta::access_context::current())) {
    if (std::meta::type_of(candidate) != std::meta::type_of(member))
      continue;
    if (candidate == member) {
      found = true;
      break;
    }
    ++seen;
  }
  if (!found)
    broken::rowOfNamesAMemberOfSynthetics();
  // NOTE: A third table adds its arm here and its count in tables.hpp.
  if constexpr (dictline)
    return Row{kDictlineRows + seen};
  else
    return Row{kInflectsRows + seen};
}

template <typename Entry> consteval std::uint32_t syntheticCount() {
  std::uint32_t count = 0;
  for (const auto member : std::meta::nonstatic_data_members_of(
           ^^Synthetics, std::meta::access_context::current()))
    if (std::meta::type_of(member) == ^^Entry)
      ++count;
  return count;
}
} // namespace internal

inline constexpr std::uint32_t kSyntheticDictlineRows =
    internal::syntheticCount<DictlineEntry>();
inline constexpr std::uint32_t kSyntheticInflectsRows =
    internal::syntheticCount<InflectsEntry>();

} // namespace wl_stable::expand
