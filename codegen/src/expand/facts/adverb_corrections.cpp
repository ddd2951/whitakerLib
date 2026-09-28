#include <cstddef>
#include <type_traits>
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
  return std::visit(
      [&](const auto& origin) -> Form {
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(origin)>,
                                     Joined>)
          return {source.spelling, Joined{origin.entry, adverb, origin.column}};
        else
          return {source.spelling, UniqueAdverb{origin.entry, adverb}};
      },
      source.origin);
}

void correctGroup(const std::vector<Form>& forms, std::size_t begin,
                  std::size_t end,
                  std::vector<AdverbCorrections::Correction>& out) {
  for (std::size_t i = begin; i < end; ++i)
    if (reads<latin::inflected::Adverb>(forms[i]))
      return;

  for (std::size_t k = end; k-- > begin;) {
    const Reading reading = readingOf(forms[k].origin);
    const auto* inflected =
        std::get_if<latin::inflected::Adjective>(&reading.inflection);
    if (inflected == nullptr)
      continue;
    if (inflected->caseOf != latin::Case::VOC ||
        inflected->number != latin::Number::S ||
        inflected->gender != latin::Gender::M)
      continue;

    const auto& declared = std::get<latin::Adjective>(reading.entry);
    const latin::Comparison degree =
        internal::adjectiveDegree(declared, reading.key);
    if (degree == latin::Comparison::POS) {
      if (declared.declension.value != 1 ||
          declared.declensionVariant.value != 1)
        continue;
    } else if (degree != latin::Comparison::SUPER) {
      continue;
    }

    std::size_t first = k;
    while (first > begin &&
           reads<latin::inflected::Adjective>(forms[first - 1]))
      --first;

    const InflectsRow adverb = degree == latin::Comparison::SUPER
                                   ? word::kAdverbSuperlative
                                   : word::kAdverbPositive;
    out.push_back({end - 1, corrected(forms[first], adverb)});
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
      while (end < forms.size() &&
             spellingEqual(forms[end].spelling, forms[begin].spelling))
        ++end;
      correctGroup(forms, begin, end, corrections.byLetter[letter]);
      begin = end;
    }
  }
  return corrections;
}
