#include "execution_steps/steps.hpp"

#include "storage/states.hpp"
#include "storage/below.hpp"
#include "storage/root.hpp"
#include "storage/result.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <utility>
#include <vector>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::packSharedStemSlots() {
  using storage::below;
  using storage::result;
  using storage::root;
  using storage::states;
  std::map<std::pair<std::uint32_t, std::vector<unsigned>>, unsigned> groups{};
  std::vector<unsigned> slotState{root}, slotOffset{0};
  for (std::size_t i = 0; i < slotState.size(); ++i) {
    const auto& s = states[slotState[i]];
    if (s.kids.empty())
      continue;
    if (groups.try_emplace({s.mask, s.kids}, static_cast<unsigned>(slotState.size())).second) {
      unsigned offset = 0;
      for (const auto kid : s.kids) {
        slotState.push_back(kid);
        slotOffset.push_back(offset);
        offset += below[kid];
      }
    }
  }
  for (std::size_t i = 0; i < slotState.size(); ++i) {
    const auto& s = states[slotState[i]];
    const unsigned first = s.kids.empty() ? 0 : groups.at({s.mask, s.kids});
    assert(!(first >> rel::kFrontFirstBits) && !(slotOffset[i] >> rel::kFrontOffsetBits));
    result.sharedFront.push_back(rel::frontSlot(s.mask, first, slotOffset[i], s.stem));
  }
}
