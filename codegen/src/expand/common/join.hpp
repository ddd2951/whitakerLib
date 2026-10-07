#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

#include "expand/common/licensing.hpp"
#include "expand/common/stem_column.hpp"
#include "types/grammar.hpp"
#include "word/word.hpp"

namespace expand::internal {

struct Licensed {
  DictlineRow entry;
  const word::DictlineEntry& dictionary;
  InflectsRow inflection;
  const word::InflectsEntry& ending;
  latin::StemKey column;
  std::string_view stem;
  std::string_view suffix;
};

// NOTE: Every dictionary row is crossed, source and synthetic. Only source inflection rows are: a synthetic inflection
//  is reached by the pass that made it, never by the join.
template <typename Visit> void forEachLicensed(Visit&& visit) {
  std::array<std::vector<std::uint32_t>, kInflectionCount> byAlternative;
  for (std::uint32_t row = 0; row < source::scheme::inflects::kEntriesPerFile; ++row)
    byAlternative[word::entry(InflectsRow{row}).grammar.index()].push_back(row);

  for (std::uint32_t d = 0; d < word::kDictlineWords; ++d) {
    const DictlineRow entryRow{d};
    const word::DictlineEntry& dictionary = word::entry(entryRow);
    const std::size_t alternative = dictionary.grammar.index();

    for (std::size_t i = 0; i < kInflectionCount; ++i) {
      if (!kPairs[alternative][i])
        continue;
      for (const std::uint32_t f : byAlternative[i]) {
        const InflectsRow inflectionRow{f};
        const word::InflectsEntry& ending = word::entry(inflectionRow);
        if (!licenses(dictionary.grammar, ending.grammar))
          continue;

        const latin::StemKey column = columnForKey(dictionary, ending.stemKey);
        const word::Stem stem = stemAt(dictionary, column);
        if (!stem)
          continue;

        const std::string_view suffix = ending.ending.substr(0, ending.characterCount.value);

        visit(Licensed{entryRow, dictionary, inflectionRow, ending, column, *stem, suffix});
      }
    }
  }
}

} // namespace expand::internal
