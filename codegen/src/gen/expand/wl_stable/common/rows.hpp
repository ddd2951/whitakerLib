#pragma once

#include "gen/expand/wl_stable/common/form.hpp"
#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "gen/expand/wl_stable/common/tables.hpp"
#include "types/domain.hpp"
#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::expand {

[[nodiscard]] constexpr bool isSynthetic(DictlineRow row) noexcept {
  return row.value >= kDictlineRows;
}
[[nodiscard]] constexpr bool isSynthetic(InflectsRow row) noexcept {
  return row.value >= kInflectsRows;
}

[[nodiscard]] DictlineEntry entry(const tokenized::Sources& sources,
                                  const Synthetics& synthetics,
                                  DictlineRow row);
[[nodiscard]] InflectsEntry inflection(const tokenized::Sources& sources,
                                       const Synthetics& synthetics,
                                       InflectsRow row);

[[nodiscard]] constexpr tokenized::Stem stemAt(const DictlineEntry& entry,
                                               domain::StemIndex column) {
  switch (column.value) {
  case 1:
    return entry.stem1;
  case 2:
    return entry.stem2;
  case 3:
    return entry.stem3;
  case 4:
    return entry.stem4;
  default:
    return text::StringPools::kAbsentStem;
  }
}

} // namespace wl_stable::expand
