#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "error/error.hpp"
#include "facts.hpp"
#include "expand/common/form.hpp"
#include "expand/common/join.hpp"
#include "expand/facts/candidates.hpp"

namespace {

struct Placed {
  std::size_t at;
  std::size_t length;
  std::uint8_t letter;
  expand::Joined origin;
};

} // namespace

// NOTE: Views into the pool are taken only after the last append; the pool is never sized ahead.
expand::Candidates expand::candidates() {
  const auto bucket = [](std::string_view stem, std::string_view suffix) -> int {
    const std::string_view head = stem.empty() ? suffix : stem;
    if (head.empty())
      return -1;
    const int index = facts::letterIndex(head.front());
    if (index < 0)
      error::invalid("stem or ending '" + std::string{head} + "' outside the alphabet");
    return index;
  };

  Candidates candidates;
  std::vector<Placed> placed;
  internal::forEachLicensed([&](const internal::Licensed& j) {
    const int index = bucket(j.stem, j.suffix);
    if (index < 0)
      return;
    placed.push_back({candidates.spellings.size(), j.stem.size() + j.suffix.size(), static_cast<std::uint8_t>(index),
                      Joined{j.entry, j.inflection, j.column}});
    candidates.spellings.insert(candidates.spellings.end(), j.stem.begin(), j.stem.end());
    candidates.spellings.insert(candidates.spellings.end(), j.suffix.begin(), j.suffix.end());
  });

  const std::string_view pool{candidates.spellings.data(), candidates.spellings.size()};
  for (const Placed& p : placed)
    candidates.byLetter[p.letter].push_back(Form{pool.substr(p.at, p.length), p.origin});
  return candidates;
}
