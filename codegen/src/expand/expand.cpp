#include "expand.hpp"

#include "error/error.hpp"
#include "facts.hpp"
#include "expand/facts/adverb_corrections.hpp"
#include "expand/facts/candidates.hpp"
#include "expand/facts/forms.hpp"
#include "expand/facts/readings.hpp"
#include "expand/facts/slice.hpp"
#include "expand/facts/sorted.hpp"
#include "expand/facts/swept.hpp"
#include "expand/facts/uniques.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <print>
#include <vector>

namespace {

struct Kept {
  expand::Candidates candidates;
  expand::Forms forms;
  expand::Readings readings;
  expand::Swept swept;
};

Kept kept;

std::array<std::size_t, facts::kLetterCount> starts{};
std::vector<std::uint8_t> letterOf;

struct At {
  std::uint8_t letter;
  std::size_t position;
};

At at(expand::ResultIndex index) {
  const std::uint8_t letter = letterOf[index.value];
  return {letter, index.value - starts[letter]};
}

} // namespace

void expand::init() {
  const Slice slice = expand::slice();
  kept.candidates = candidates(slice);
  const Sorted sorted = expand::sorted(kept.candidates, uniques(slice));
  kept.forms = forms(sorted, adverbCorrections(sorted));
  kept.readings = readings(kept.forms);
  kept.swept = swept(kept.forms, kept.readings);
  std::println("expand has run");
  report(kept.forms);
  report(kept.swept);
  std::println("expand post has run");

  for (std::uint8_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::size_t count = kept.forms.byLetter[letter].size();
    if (kept.readings.byLetter[letter].size() != count ||
        kept.swept.byLetter[letter].size() != count)
      error::fatal("expand: forms, readings and fates are not aligned");
    starts[letter] = letterOf.size();
    letterOf.insert(letterOf.end(), count, letter);
  }
}

std::size_t expand::resultCount() { return letterOf.size(); }

std::string_view expand::Result::spelling() const {
  const At a = at(index);
  return kept.forms.byLetter[a.letter][a.position].spelling;
}
expand::Origin expand::Result::origin() const {
  const At a = at(index);
  return kept.forms.byLetter[a.letter][a.position].origin;
}
expand::Reading expand::Result::reading() const {
  const At a = at(index);
  return kept.readings.byLetter[a.letter][a.position];
}
expand::Fate expand::Result::fate() const {
  const At a = at(index);
  return kept.swept.byLetter[a.letter][a.position];
}
