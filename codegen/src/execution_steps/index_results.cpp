#include "execution_steps/steps.hpp"

#include "facts.hpp"
#include "storage/kept.hpp"
#include "storage/starts.hpp"
#include "storage/total.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>

void execution_steps::expansion::indexResults() {
  using storage::kept;
  using storage::starts;
  using storage::total;
  for (std::uint8_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::size_t count = kept.forms.byLetter[letter].size();
    assert(kept.readings.byLetter[letter].size() == count && kept.swept.byLetter[letter].size() == count);
    starts[letter] = total;
    total += count;
  }
}
