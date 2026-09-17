#include <cstddef>
#include <print>
#include <vector>

#include "facts.hpp"
#include "gen/expand/wl_stable/facts/forms.hpp"

// NOTE: Sorted, with every correction placed after the form it names.
wl_stable::expand::Forms
wl_stable::expand::enrolled::forms(const Sorted& sorted,
                                   const AdverbCorrections& corrections) {
  Forms result;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::vector<Form>& forms = sorted.byLetter[letter];
    const auto& added = corrections.byLetter[letter];
    std::vector<Form>& out = result.byLetter[letter];
    out.reserve(forms.size() + added.size());

    std::size_t next = 0;
    for (std::size_t i = 0; i < forms.size(); ++i) {
      out.push_back(forms[i]);
      while (next < added.size() && added[next].after == i)
        out.push_back(added[next++].form);
    }
  }
  return result;
}

void wl_stable::expand::enrolled::report(const Forms& forms) {
  std::size_t total = 0;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::size_t count = forms.byLetter[letter].size();
    if (count == 0)
      continue;
    total += count;
    std::println("forms for '{}': {}", facts::kLatinLetters[letter], count);
  }
  std::println("forms total: {}", total);
}
