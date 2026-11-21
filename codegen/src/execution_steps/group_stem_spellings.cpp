#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "storage/classes.hpp"
#include "storage/ending_runs.hpp"
#include "storage/rows.hpp"
#include "storage/stem_runs.hpp"
#include "storage/stems.hpp"

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::groupStemSpellings() {
  using storage::classes;
  using storage::endingRuns;
  using storage::rows;
  using storage::stemRuns;
  using storage::stems;
  for (unsigned i = 0; i < stems.size(); ++i) {
    const auto [it, fresh] = stemRuns.try_emplace(emitter::normalize(stems[i].text), rel::Run{i, 0});
    ++it->second.count;
  }
  for (const auto& [ids, keep] : classes)
    for (const auto id : ids)
      endingRuns.try_emplace(rows[id].suffix);
}
