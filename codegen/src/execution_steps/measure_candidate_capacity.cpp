#include "execution_steps/steps.hpp"

#include "storage/candidate_capacity.hpp"
#include "storage/candidate_counts.hpp"
#include "storage/lengths.hpp"
#include <algorithm>

void execution_steps::lookup_tables::measureCandidateCapacity() {
  using storage::candidateCapacity;
  using storage::candidateCounts;
  using storage::lengths;
  for (const auto& [text, count] : candidateCounts)
    candidateCapacity = std::max(candidateCapacity, count);
  candidateCounts.clear();
  lengths.clear();
}
