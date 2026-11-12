#include "execution_steps/steps.hpp"

#include "storage/ending_ids.hpp"
#include "storage/ending_runs.hpp"
#include "storage/endings.hpp"
#include "storage/reversed.hpp"

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::numberEndings() {
  using storage::endingIds;
  using storage::endingRuns;
  using storage::endings;
  using storage::reversed;
  for (auto& [text, run] : endingRuns) {
    run = {endings++, 1};
    endingIds.emplace(text, run.first);
    reversed.emplace(std::string(text.rbegin(), text.rend()), run);
  }
}
