#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "storage/stems.hpp"
#include <algorithm>
#include <tuple>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::sortLookupStems() {
  using storage::SourceStem;
  using storage::stems;
  std::ranges::sort(stems, [](const SourceStem& a, const SourceStem& b) {
    return std::tuple(emitter::normalize(a.text), a.klass == rel::kNoLookupClass, a.dictionary, a.column) <
           std::tuple(emitter::normalize(b.text), b.klass == rel::kNoLookupClass, b.dictionary, b.column);
  });
}
