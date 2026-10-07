#include "expand.hpp"

#include "facts.hpp"
#include "expand/facts/adverb_corrections.hpp"
#include "expand/facts/candidates.hpp"
#include "expand/facts/forms.hpp"
#include "expand/facts/readings.hpp"
#include "expand/facts/sorted.hpp"
#include "expand/facts/swept.hpp"
#include "expand/facts/uniques.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace {

struct Kept {
  // NOTE: Form::spelling depends on this, don't let it dangle.
  expand::Candidates candidates;
  expand::Forms forms;
  expand::Readings readings;
  expand::Swept swept;
};

Kept kept;

std::array<std::size_t, facts::kLetterCount> starts{};
std::size_t total = 0;

struct At {
  std::uint8_t letter;
  std::size_t position;
};

At at(expand::ResultIndex index) {
  const auto letter = static_cast<std::uint8_t>(std::ranges::upper_bound(starts, index.value) - starts.begin() - 1);
  return {letter, index.value - starts[letter]};
}

} // namespace

void expand::init() {
  kept.candidates = candidates();
  const Sorted sorted = expand::sorted(kept.candidates, uniques());
  kept.forms = forms(sorted, adverbCorrections(sorted));
  kept.readings = readings(kept.forms);
  kept.swept = swept(kept.forms, kept.readings);

  for (std::uint8_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::size_t count = kept.forms.byLetter[letter].size();
    assert(kept.readings.byLetter[letter].size() == count && kept.swept.byLetter[letter].size() == count);
    starts[letter] = total;
    total += count;
  }
}

std::size_t expand::resultCount() { return total; }

std::string_view expand::spelling(ResultIndex result) {
  const At a = at(result);
  return kept.forms.byLetter[a.letter][a.position].spelling;
}
expand::Origin expand::origin(ResultIndex result) {
  const At a = at(result);
  return kept.forms.byLetter[a.letter][a.position].origin;
}
expand::Reading expand::reading(ResultIndex result) {
  const At a = at(result);
  return kept.readings.byLetter[a.letter][a.position];
}
expand::Fate expand::fate(ResultIndex result) {
  const At a = at(result);
  return kept.swept.byLetter[a.letter][a.position];
}
