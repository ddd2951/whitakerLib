#include "execution_steps/steps.hpp"

#include "helpers/rule.hpp"
#include "storage/by_stem.hpp"
#include "storage/candidate_counts.hpp"
#include "storage/unique_base.hpp"

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::addUniqueRules() {
  using helpers::rule;
  using storage::byStem;
  using storage::candidateCounts;
  using storage::uniqueBase;
  for (unsigned i = 0; i < source::scheme::uniques::kEntriesPerFile; ++i) {
    ++candidateCounts[emitter::normalize(word::entry(word::UniquesRow{i}).form)];
    byStem[{uniqueBase + i, 1}].push_back(rule(expand::Unique{word::UniquesRow{i}}, i, "", "", -1));
  }
}
