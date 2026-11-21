#include "execution_steps/steps.hpp"

#include "storage/front.hpp"
#include "storage/states.hpp"
#include "storage/below.hpp"
#include "storage/root.hpp"
#include <bit>
#include <map>
#include <utility>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::canonicalizeStemTrie() {
  using storage::front;
  using storage::State;
  // INFO: Equal subtrees share a state and equal sibling groups share slots. Runs are contiguous in spelling order, so
  //  counting the stems before a state along the path finds its run.
  std::map<State, unsigned> ids{};
  std::vector<State> states{};
  std::vector<unsigned> below{};
  const auto canon = [&](auto&& self, const unsigned at) -> unsigned {
    const auto n = front[at];
    State s{n.mask, !rel::empty(n.run), {}};
    for (unsigned i = 0, k = static_cast<unsigned>(std::popcount(n.mask)); i < k; ++i)
      s.kids.push_back(self(self, n.first + i));
    if (const auto it = ids.find(s); it != ids.end())
      return it->second;
    unsigned total = s.stem ? 1 : 0;
    for (const auto kid : s.kids)
      total += below[kid];
    const auto id = static_cast<unsigned>(states.size());
    ids.emplace(s, id);
    states.push_back(s);
    below.push_back(total);
    return id;
  };
  const auto root = canon(canon, 0);
  storage::states = std::move(states);
  storage::below = std::move(below);
  storage::root = std::move(root);
}
