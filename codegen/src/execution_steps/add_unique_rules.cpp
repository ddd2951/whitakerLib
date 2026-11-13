#include "execution_steps/steps.hpp"

#include "helpers/rule.hpp"
#include "storage/by_stem.hpp"
#include "storage/candidate_counts.hpp"
#include "word/word.hpp"

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::addUniqueRules() {
  using helpers::rule;
  using storage::byStem;
  using storage::candidateCounts;
  for (unsigned i = 0; i < source::scheme::uniques::kEntriesPerFile; ++i) {
    ++candidateCounts[emitter::normalize(word::entry(word::UniquesRow{i}).form)];
    byStem[{word::kUniqueBase + i, 1}].push_back(rule(expand::Unique{word::UniquesRow{i}}, i, "", "", -1));
  }
}
