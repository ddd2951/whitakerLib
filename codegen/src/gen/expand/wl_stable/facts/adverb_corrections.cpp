#include <cstddef>
#include <type_traits>
#include <variant>
#include <vector>

#include "facts.hpp"
#include "gen/expand/wl_stable/common/reading.hpp"
#include "gen/expand/wl_stable/common/stem_column.hpp"
#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "gen/expand/wl_stable/facts/adverb_corrections.hpp"

namespace {

using namespace wl_stable::expand;

template <typename Part>
[[nodiscard]] bool reads(const tokenized::Sources& sources,
                         const Synthetics& synthetics, const Form& form) {
  return std::holds_alternative<Part>(
      readingOf(sources, synthetics, form.origin).inflection);
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

// NOTE: One spelling group: forms[begin, end).
void correctGroup(const tokenized::Sources& sources,
                  const Synthetics& synthetics, const std::vector<Form>& forms,
                  std::size_t begin, std::size_t end,
                  std::vector<AdverbCorrections::Correction>& out) {
  for (std::size_t i = begin; i < end; ++i)
    if (reads<tokenized::inflected::Adverb>(sources, synthetics, forms[i]))
      return;

  for (std::size_t k = end; k-- > begin;) {
    const Reading reading = readingOf(sources, synthetics, forms[k].origin);
    const auto* inflected =
        std::get_if<tokenized::inflected::Adjective>(&reading.inflection);
    if (inflected == nullptr)
      continue;
    if (inflected->caseOf != TypeCase::VOC ||
        inflected->number != TypeNumber::S ||
        inflected->gender != TypeGender::M)
      continue;

    const auto& declared = std::get<tokenized::Adjective>(reading.entry);
    const TypeComparison degree =
        internal::adjectiveDegree(declared, reading.key);
    if (degree == TypeComparison::POS) {
      if (declared.declension.value != 1 ||
          declared.declensionVariant.value != 1)
        continue;
    } else if (degree != TypeComparison::SUPER) {
      continue;
    }

    // NOTE: The source is the first of the adjective run this reading sits in.
    std::size_t first = k;
    while (first > begin && reads<tokenized::inflected::Adjective>(
                                sources, synthetics, forms[first - 1]))
      --first;

    const InflectsRow adverb =
        degree == TypeComparison::SUPER
            ? internal::rowOf<InflectsRow>(^^Synthetics::adverbSuperlative)
            : internal::rowOf<InflectsRow>(^^Synthetics::adverbPositive);
    out.push_back({end - 1, corrected(forms[first], adverb)});
  }
}

} // namespace

wl_stable::expand::AdverbCorrections
wl_stable::expand::enrolled::adverbCorrections(
    const tokenized::Sources& sources, const Synthetics& synthetics,
    const Sorted& sorted) {
  AdverbCorrections corrections;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::vector<Form>& forms = sorted.byLetter[letter];
    std::size_t begin = 0;
    while (begin < forms.size()) {
      std::size_t end = begin;
      while (end < forms.size() &&
             spellingEqual(forms[end].spelling, forms[begin].spelling))
        ++end;
      correctGroup(sources, synthetics, forms, begin, end,
                   corrections.byLetter[letter]);
      begin = end;
    }
  }
  return corrections;
}
