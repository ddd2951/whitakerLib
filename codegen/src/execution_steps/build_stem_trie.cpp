#include "execution_steps/steps.hpp"

#include "helpers/trie.hpp"
#include "storage/front.hpp"
#include "storage/stem_runs.hpp"
#include <utility>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::buildStemTrie() {
  using helpers::trie;
  using storage::stemRuns;
  const auto front = trie(stemRuns);
  storage::front = std::move(front);
}
