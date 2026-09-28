#pragma once

#include "expand/common/form.hpp"
#include "expand/common/tables.hpp"
#include "latin.hpp"
#include "types/grammar.hpp"

namespace expand {

[[nodiscard]] DictlineEntry entry(DictlineRow row);
[[nodiscard]] InflectsEntry inflection(InflectsRow row);

[[nodiscard]] constexpr word::Stem stemAt(const DictlineEntry& entry,
                                          latin::StemKey column) {
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
    return word::kAbsentStem;
  }
}

} // namespace expand
