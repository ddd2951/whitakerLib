#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "error/error.hpp"
#include "facts.hpp"
#include "gen/expand/wl_stable/common/form.hpp"
#include "gen/expand/wl_stable/common/join.hpp"
#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "gen/expand/wl_stable/facts/candidates.hpp"

namespace {

// NOTE: Where a spelling will be once the pool is complete.
struct Placed {
  std::size_t at;
  std::size_t length;
  std::uint8_t letter;
  wl_stable::expand::Joined origin;
};

} // namespace

// NOTE: Views into the pool are taken only after the last append; the pool
// is never sized ahead.
wl_stable::expand::Candidates
wl_stable::expand::enrolled::candidates(const tokenized::Sources& sources,
                                        const Synthetics& synthetics,
                                        const Slice& slice) {
  const auto want = wanted(slice.letters);
  const auto bucket = [&want](std::string_view stem,
                              std::string_view suffix) -> int {
    // NOTE: An empty stem with an empty ending spells nothing.
    const std::string_view head = stem.empty() ? suffix : stem;
    if (head.empty())
      return -1;
    const int index = facts::letterIndex(head.front());
    if (index < 0)
      error::fatal("stem or ending begins outside the alphabet: " +
                   std::string{head});
    return want[static_cast<std::size_t>(index)] ? index : -1;
  };

  Candidates candidates;
  std::vector<Placed> placed;
  internal::forEachLicensed(
      sources, synthetics, [&](const internal::Licensed& j) {
        const int index = bucket(j.stem, j.suffix);
        if (index < 0)
          return;
        placed.push_back({candidates.spellings.size(),
                          j.stem.size() + j.suffix.size(),
                          static_cast<std::uint8_t>(index),
                          Joined{j.entry, j.inflection, j.column}});
        candidates.spellings.insert(candidates.spellings.end(),
                                    j.stem.begin(), j.stem.end());
        candidates.spellings.insert(candidates.spellings.end(),
                                    j.suffix.begin(), j.suffix.end());
      });

  const std::string_view pool{candidates.spellings.data(),
                              candidates.spellings.size()};
  for (const Placed& p : placed)
    candidates.byLetter[p.letter].push_back(
        Form{pool.substr(p.at, p.length), p.origin});
  return candidates;
}
