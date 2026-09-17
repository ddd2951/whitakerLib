#include <algorithm>
#include <cstddef>
#include <variant>

#include "facts.hpp"
#include "gen/expand/wl_stable/facts/sorted.hpp"

namespace {

using namespace wl_stable::expand;

// NOTE: Unique readings precede joined ones; a kind the sort never sees
// still orders, so the comparator is total over Origin.
bool originLess(const Origin& a, const Origin& b) {
  const bool ua = std::holds_alternative<Unique>(a);
  const bool ub = std::holds_alternative<Unique>(b);
  if (ua != ub)
    return ua;
  if (a.index() != b.index())
    return a.index() < b.index();
  if (ua)
    return std::get<Unique>(a).entry.value < std::get<Unique>(b).entry.value;
  if (const auto* ja = std::get_if<Joined>(&a)) {
    const Joined& jb = std::get<Joined>(b);
    if (ja->entry != jb.entry)
      return ja->entry.value < jb.entry.value;
    return ja->inflection.value < jb.inflection.value;
  }
  const UniqueAdverb& xa = std::get<UniqueAdverb>(a);
  const UniqueAdverb& xb = std::get<UniqueAdverb>(b);
  if (xa.entry != xb.entry)
    return xa.entry.value < xb.entry.value;
  return xa.inflection.value < xb.inflection.value;
}

bool formLess(const Form& a, const Form& b) {
  if (a.spelling.size() != b.spelling.size())
    return a.spelling.size() < b.spelling.size();
  if (!spellingEqual(a.spelling, b.spelling))
    return spellingLess(a.spelling, b.spelling);
  return originLess(a.origin, b.origin);
}

} // namespace

wl_stable::expand::Sorted
wl_stable::expand::enrolled::sorted(const Candidates& candidates,
                                    const Uniques& uniques) {
  Sorted sorted;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    auto& forms = sorted.byLetter[letter];
    forms.reserve(candidates.byLetter[letter].size() +
                  uniques.byLetter[letter].size());
    forms.insert(forms.end(), candidates.byLetter[letter].begin(),
                 candidates.byLetter[letter].end());
    forms.insert(forms.end(), uniques.byLetter[letter].begin(),
                 uniques.byLetter[letter].end());
    std::ranges::sort(forms, formLess);
  }
  return sorted;
}
