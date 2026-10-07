#include <cstddef>
#include <cstdint>
#include <string_view>

#include "facts.hpp"
#include "expand/facts/uniques.hpp"
#include "word/word.hpp"

// NOTE: A unique licenses only its finished form; the spelling is the source's.
expand::Uniques expand::uniques() {
  Uniques uniques;
  for (std::uint32_t row = 0; row < source::scheme::uniques::kEntriesPerFile; ++row) {
    const std::string_view form = word::entry(UniquesRow{row}).form;
    const auto index = static_cast<std::size_t>(facts::letterIndex(form.front()));
    uniques.byLetter[index].push_back(Form{form, Unique{UniquesRow{row}}});
  }
  return uniques;
}
