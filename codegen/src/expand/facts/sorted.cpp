#include <algorithm>
#include <cstddef>
#include <variant>

#include "facts.hpp"
#include "expand/facts/sorted.hpp"

namespace {

using namespace expand;

bool originLess(const Origin& a, const Origin& b) {
  const bool ua = std::holds_alternative<Unique>(a);
  const bool ub = std::holds_alternative<Unique>(b);
  if (ua != ub)
    return ua;
  if (a.index() != b.index())
    return a.index() < b.index();
  if (ua)
    return std::get<Unique>(a).entry.value < std::get<Unique>(b).entry.value;
  const Joined& ja = std::get<Joined>(a);
  const Joined& jb = std::get<Joined>(b);
  return ja.entry != jb.entry ? ja.entry.value < jb.entry.value : ja.inflection.value < jb.inflection.value;
}

bool formLess(const Form& a, const Form& b) {
  if (a.spelling.size() != b.spelling.size())
    return a.spelling.size() < b.spelling.size();
  if (!spellingEqual(a.spelling, b.spelling))
    return spellingLess(a.spelling, b.spelling);
  return originLess(a.origin, b.origin);
}

} // namespace

expand::Sorted expand::sorted(const Candidates& candidates, const Uniques& uniques) {
  Sorted sorted;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    auto& forms = sorted.byLetter[letter];
    forms.reserve(candidates.byLetter[letter].size() + uniques.byLetter[letter].size());
    forms.insert(forms.end(), candidates.byLetter[letter].begin(), candidates.byLetter[letter].end());
    forms.insert(forms.end(), uniques.byLetter[letter].begin(), uniques.byLetter[letter].end());
    std::ranges::sort(forms, formLess);
  }
  return sorted;
}
