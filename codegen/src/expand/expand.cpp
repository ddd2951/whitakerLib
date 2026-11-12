#include "expand.hpp"

#include "execution_steps/steps.hpp"
#include "storage/kept.hpp"
#include "storage/starts.hpp"
#include "storage/total.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace {

using storage::kept;

using storage::starts;
using storage::total;

struct At {
  std::uint8_t letter{};
  std::size_t position{};
};
static_assert(std::is_trivially_copyable_v<At>);

At at(const expand::ResultIndex index) {
  const auto letter = static_cast<std::uint8_t>(std::ranges::upper_bound(starts, index.value) - starts.begin() - 1);
  return {letter, index.value - starts[letter]};
}

} // namespace

std::size_t expand::resultCount() { return total; }

std::string_view expand::spelling(const ResultIndex result) {
  const At a = at(result);
  return kept.forms.byLetter[a.letter][a.position].spelling;
}
expand::Origin expand::origin(const ResultIndex result) {
  const At a = at(result);
  return kept.forms.byLetter[a.letter][a.position].origin;
}
expand::Reading expand::reading(const ResultIndex result) {
  const At a = at(result);
  return kept.readings.byLetter[a.letter][a.position];
}
expand::Fate expand::fate(const ResultIndex result) {
  const At a = at(result);
  return kept.swept.byLetter[a.letter][a.position];
}
