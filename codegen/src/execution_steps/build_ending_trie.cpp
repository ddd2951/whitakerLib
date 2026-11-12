#include "execution_steps/steps.hpp"

#include "helpers/trie.hpp"
#include "storage/back.hpp"
#include "storage/reversed.hpp"
#include <utility>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::buildEndingTrie() {
  using helpers::trie;
  using storage::reversed;
  const auto back = trie(reversed);
  storage::back = std::move(back);
}
