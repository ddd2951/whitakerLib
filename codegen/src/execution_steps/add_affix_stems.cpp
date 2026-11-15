#include "execution_steps/steps.hpp"

#include "expand/common/stem_column.hpp"
#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "storage/stems.hpp"
#include "word/word.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::addAffixStems() {
  using storage::stems;
  // TODO: Derive stem order once.
  // NOTE: Plain and affix lookups can use different stems.
  // NOTE: buildLookupClasses orders stems uniquely by dictionary and physical column.
  const auto address = [](const storage::SourceStem& stem) { return std::pair{stem.dictionary, stem.column}; };
  assert(std::ranges::adjacent_find(stems, std::greater_equal{}, address) == stems.end());
  const std::size_t originalSize = stems.size();
  std::size_t existing{};
  for (unsigned dictionary = 0; dictionary < word::kDictlineWords; ++dictionary) {
    const auto& entry = word::entry(word::DictlineRow{dictionary});
    for (unsigned column = 1; column <= facts::kStemColumnCount; ++column) {
      const auto text = expand::stemAt(entry, latin::StemKey{static_cast<unsigned char>(column)});
      const auto key = expand::internal::stemKeyOfColumn(entry, static_cast<std::uint8_t>(column));
      // NOTE: Representative keys are distinct within a dictionary word.
      if (!text || text->empty() || !key)
        continue;

      const auto target = std::pair{dictionary, column - 1};
      while (existing < originalSize && address(stems[existing]) < target)
        ++existing;
      const bool found = existing < originalSize && address(stems[existing]) == target;
      const std::size_t index = found ? existing : stems.size();
      if (!found)
        stems.push_back({std::string(*text), dictionary, column - 1, 0, rel::kNoLookupClass});
      auto& stem = stems[index];
      assert(emitter::normalize(stem.text) == emitter::normalize(*text));
      stem.stemKey = key->value;
      stem.part = latin::partOf(entry.grammar);
    }
  }
}
