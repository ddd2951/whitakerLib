#include "execution_steps/steps.hpp"

#include "storage/front.hpp"
#include "storage/stems.hpp"
#include "storage/result.hpp"
#include <bit>
#include <cassert>
#include <cstdint>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::numberStemRuns() {
  using storage::front;
  using storage::result;
  using storage::stems;
  unsigned expected = 0;
  const auto number = [&](auto&& self, const unsigned at) -> void {
    const auto n = front[at];
    if (!rel::empty(n.run)) {
      assert(n.run.first == expected);
      result.frontRuns.push_back(static_cast<std::uint16_t>(expected));
      expected += n.run.count;
    }
    for (unsigned i = 0, k = static_cast<unsigned>(std::popcount(n.mask)); i < k; ++i)
      self(self, n.first + i);
  };
  number(number, 0);
  assert(expected <= 65535 && expected == stems.size());
  result.frontRuns.push_back(static_cast<std::uint16_t>(expected));
}
