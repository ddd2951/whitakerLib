#include "execution_steps/steps.hpp"

#include "expand/common/stem_column.hpp"
#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "storage/stems.hpp"
#include "word/word.hpp"
#include <map>
#include <set>
#include <tuple>
#include <utility>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::addAffixStems() {
  using storage::stems;
  // INFO: Affixes reach stems with no plain reading, and some plain stems are absent from the affix table.
  //  Share the union, preserving each pass's membership rather than treating those sets as identical.
  std::map<std::tuple<std::string, unsigned, unsigned>, unsigned> addresses{};
  for (unsigned i = 0; i < stems.size(); ++i)
    addresses.emplace(std::tuple{emitter::normalize(stems[i].text), stems[i].dictionary, stems[i].column}, i);
  for (unsigned dictionary = 0; dictionary < word::kDictlineWords; ++dictionary) {
    const auto& entry = word::entry(word::DictlineRow{dictionary});
    std::set<std::pair<std::string, unsigned>> seen{};
    for (unsigned column = 1; column <= facts::kStemColumnCount; ++column) {
      const auto text = expand::stemAt(entry, latin::StemKey{static_cast<unsigned char>(column)});
      const auto key = expand::internal::stemKeyOfColumn(entry, static_cast<std::uint8_t>(column));
      if (!text || text->empty() || !key || !seen.emplace(emitter::normalize(*text), key->value).second)
        continue;
      const auto [it, fresh] = addresses.emplace(std::tuple{emitter::normalize(*text), dictionary, column - 1},
                                                 static_cast<unsigned>(stems.size()));
      if (fresh)
        stems.push_back({std::string(*text), dictionary, column - 1, 0, rel::kNoLookupClass});
      auto& stem = stems[it->second];
      stem.stemKey = key->value;
      stem.part = latin::partOf(entry.grammar);
    }
  }
}
