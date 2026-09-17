#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>
#include <vector>

#include "gen/expand/wl_stable/common/licensing.hpp"
#include "gen/expand/wl_stable/common/rows.hpp"
#include "gen/expand/wl_stable/common/stem_column.hpp"
#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "gen/expand/wl_stable/common/tables.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::expand::internal {

template <std::size_t D>
consteval std::array<bool, kInflectionCount> pairsOf() {
  return []<std::size_t... I>(std::index_sequence<I...>) {
    return std::array<bool, kInflectionCount>{kMayPair<D, I>...};
  }(std::make_index_sequence<kInflectionCount>{});
}
inline constexpr auto kPairs = []<std::size_t... D>(std::index_sequence<D...>) {
  return std::array<std::array<bool, kInflectionCount>, kGrammarCount>{
      pairsOf<D>()...};
}(std::make_index_sequence<kGrammarCount>{});

struct Licensed {
  DictlineRow entry;
  const DictlineEntry& dictionary;
  InflectsRow inflection;
  const InflectsEntry& ending;
  domain::StemIndex column;
  std::string_view stem;
  std::string_view suffix;
};

// NOTE: Every dictionary row is crossed, source and synthetic. Only source
// inflection rows are, because a synthetic inflection is reached by the pass
// that made it and never by the join!
template <typename Visit>
void forEachLicensed(const tokenized::Sources& sources,
                     const Synthetics& synthetics, Visit&& visit) {
  std::array<std::vector<std::uint32_t>, kInflectionCount> byAlternative;
  for (std::uint32_t row = 0; row < kInflectsRows; ++row)
    byAlternative[sources.inflects.grammar[row].index()].push_back(row);

  const std::uint32_t rows = kDictlineRows + kSyntheticDictlineRows;
  for (std::uint32_t d = 0; d < rows; ++d) {
    const DictlineRow entryRow{d};
    const DictlineEntry dictionary = entry(sources, synthetics, entryRow);
    const std::size_t alternative = dictionary.grammar.index();

    for (std::size_t i = 0; i < kInflectionCount; ++i) {
      if (!kPairs[alternative][i])
        continue;
      for (const std::uint32_t f : byAlternative[i]) {
        const InflectsRow inflectionRow{f};
        const InflectsEntry ending =
            inflection(sources, synthetics, inflectionRow);
        if (!licenses(dictionary.grammar, ending.grammar))
          continue;

        const domain::StemIndex column =
            columnForKey(dictionary, ending.stemKey);
        const tokenized::Stem stem = stemAt(dictionary, column);
        if (tokenized::stemState(stem) == tokenized::StemState::absent)
          continue;

        const std::string_view suffix =
            ending.ending.substr(0, ending.characterCount.value);

        visit(Licensed{entryRow, dictionary, inflectionRow, ending, column,
                       stem, suffix});
      }
    }
  }
}

} // namespace wl_stable::expand::internal
