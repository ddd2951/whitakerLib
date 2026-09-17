#include "rows.hpp"

#include <cstddef>
#include <meta>

#include "error/error.hpp"
#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "gen/expand/wl_stable/common/tables.hpp"

namespace {

// NOTE: The synthetic rows of one type, in the declaration order of Synthetics.
template <typename Row>
const Row& synthetic(const wl_stable::expand::Synthetics& synthetics,
                     std::uint32_t index) {
  static constexpr auto kMembers =
      std::define_static_array(std::meta::nonstatic_data_members_of(
          ^^wl_stable::expand::Synthetics,
          std::meta::access_context::current()));
  std::uint32_t seen = 0;
  template for (constexpr auto member : kMembers) {
    if constexpr (std::meta::type_of(member) == ^^Row) {
      if (seen == index)
        return synthetics.[:member:];
      ++seen;
    }
  }
  error::fatal("expand: a row names a synthetic entry that does not exist");
}

} // namespace

wl_stable::expand::DictlineEntry
wl_stable::expand::entry(const tokenized::Sources& sources,
                         const Synthetics& synthetics, DictlineRow row) {
  if (isSynthetic(row))
    return synthetic<DictlineEntry>(synthetics, row.value - kDictlineRows);
  const tokenized::Dictline& d = sources.dictline;
  const std::size_t i = row.value;
  return {d.stem1[i],     d.stem2[i],  d.stem3[i], d.stem4[i],
          d.grammar[i],   d.age[i],    d.area[i],  d.geography[i],
          d.frequency[i], d.source[i], d.senses[i]};
}

wl_stable::expand::InflectsEntry
wl_stable::expand::inflection(const tokenized::Sources& sources,
                              const Synthetics& synthetics, InflectsRow row) {
  if (isSynthetic(row))
    return synthetic<InflectsEntry>(synthetics, row.value - kInflectsRows);
  const tokenized::Inflects& f = sources.inflects;
  const std::size_t i = row.value;
  return {f.grammar[i], f.stemKey[i], f.characterCount[i],
          f.ending[i],  f.age[i],     f.frequency[i]};
}
