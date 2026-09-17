#include <cstddef>
#include <cstdint>
#include <string_view>

#include "facts.hpp"
#include "gen/expand/wl_stable/common/tables.hpp"
#include "gen/expand/wl_stable/facts/uniques.hpp"

// NOTE: A unique licenses only its finished form; the spelling is the source's.
wl_stable::expand::Uniques
wl_stable::expand::enrolled::uniques(const tokenized::Sources& sources,
                                     const Slice& slice) {
  const auto want = wanted(slice.letters);
  Uniques uniques;
  for (std::uint32_t row = 0; row < kUniquesRows; ++row) {
    const std::string_view form = sources.uniques.form[row];
    const auto index =
        static_cast<std::size_t>(facts::letterIndex(form.front()));
    if (!want[index])
      continue;
    uniques.byLetter[index].push_back(Form{form, Unique{UniquesRow{row}}});
  }
  return uniques;
}
