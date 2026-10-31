#include <algorithm>
#include <cstddef>
#include <utility>
#include <variant>
#include <vector>

#include "facts.hpp"
#include "expand/common/reading.hpp"
#include "expand/common/stem_column.hpp"
#include "expand/facts/adverb_corrections.hpp"

namespace {

using namespace expand;

template <typename Part> [[nodiscard]] bool reads(const Form& form) {
  return std::holds_alternative<Part>(readingOf(form.origin).inflection);
}

Form corrected(const Form& source, InflectsRow adverb) {
  const Joined& origin = std::get<Joined>(source.origin);
  return {source.spelling, Joined{origin.entry, adverb, origin.column}};
}

void correctGroup(const std::vector<Form>& forms, std::size_t begin, std::size_t end,
                  std::vector<AdverbCorrections::Correction>& out) {
  for (std::size_t i = begin; i < end; ++i)
    if (reads<latin::inflected::Adverb>(forms[i]))
      return;

  // INFO: Fix_Adverb (WORDS reference 0b32fb9, Ada bug #4): one adverb per qualifying record, from that record; the
  //  readings of one record that make the same adverb make it once.
  std::vector<std::pair<DictlineRow, InflectsRow>> made;
  for (std::size_t k = end; k-- > begin;) {
    // INFO: Fix_Adverb leaves a unique alone (WORDS reference 5c9a9c0, Ada bug #5).
    if (std::holds_alternative<Unique>(forms[k].origin))
      continue;
    const Reading reading = readingOf(forms[k].origin);
    const auto* inflected = std::get_if<latin::inflected::Adjective>(&reading.inflection);
    if (inflected == nullptr)
      continue;
    if (inflected->caseOf != latin::Case::VOC || inflected->number != latin::Number::S ||
        inflected->gender != latin::Gender::M)
      continue;

    const auto& declared = std::get<latin::Adjective>(reading.entry);
    const latin::Comparison degree = internal::adjectiveDegree(declared, reading.key);
    if (degree == latin::Comparison::POS) {
      if (declared.declension.value != 1 || declared.declensionVariant.value != 1)
        continue;
    } else if (degree != latin::Comparison::SUPER) {
      continue;
    }

    const InflectsRow adverb = degree == latin::Comparison::SUPER ? word::kAdverbSuperlative : word::kAdverbPositive;
    const DictlineRow record = std::get<Joined>(forms[k].origin).entry;
    if (std::ranges::contains(made, std::pair{record, adverb}))
      continue;
    made.emplace_back(record, adverb);
    out.push_back({end - 1, corrected(forms[k], adverb)});
  }
}

} // namespace

expand::AdverbCorrections expand::adverbCorrections(const Sorted& sorted) {
  AdverbCorrections corrections;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::vector<Form>& forms = sorted.byLetter[letter];
    std::size_t begin = 0;
    while (begin < forms.size()) {
      std::size_t end = begin;
      while (end < forms.size() && spellingEqual(forms[end].spelling, forms[begin].spelling))
        ++end;
      correctGroup(forms, begin, end, corrections.byLetter[letter]);
      begin = end;
    }
  }
  return corrections;
}
