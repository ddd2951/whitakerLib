#pragma once

#include "facts.hpp"
#include "expand/common/reading.hpp"
#include "expand/common/stem_column.hpp"
#include "latin.hpp"
#include "types/grammar.hpp"
#include "util/reflect_util.hpp"

#include <meta>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace emitter {

[[nodiscard]] inline std::string normalize(std::string_view text) {
  std::string result{text};
  for (char& c : result)
    c = facts::foldLetter(c);
  return result;
}

[[nodiscard]] inline latin::Comparison advComparisonFromKey(latin::StemKey key) noexcept {
  switch (key.value) {
  case 1:
    return latin::Comparison::POS;
  case 2:
    return latin::Comparison::COMP;
  case 3:
    return latin::Comparison::SUPER;
  default:
    return latin::Comparison::X;
  }
}

[[nodiscard]] inline latin::NumeralSort numeralSortFromKey(latin::StemKey key) noexcept {
  switch (key.value) {
  case 1:
    return latin::NumeralSort::CARD;
  case 2:
    return latin::NumeralSort::ORD;
  case 3:
    return latin::NumeralSort::DIST;
  case 4:
    return latin::NumeralSort::ADVERB;
  default:
    return latin::NumeralSort::X;
  }
}

[[nodiscard]] inline bool isDegree(latin::Comparison comparison) noexcept {
  return comparison == latin::Comparison::POS || comparison == latin::Comparison::COMP ||
         comparison == latin::Comparison::SUPER;
}

using latin::partOf;

// INFO: Wildcard matches retain the dictionary's concrete grammar values.
[[nodiscard]] inline latin::Analysis analysisOf(const latin::Entry& dictionary, const latin::Inflection& inflection,
                                                latin::StemKey key) {
  latin::Analysis analysis{.part = partOf(inflection)};
  const bool adverbFix = analysis.part == latin::Part::ADV && partOf(dictionary) != latin::Part::ADV;
  if (!adverbFix)
    std::visit(
        [&](const auto& value) {
          if constexpr (requires { value.declension; }) {
            analysis.which = value.declension.value;
            analysis.variant = value.declensionVariant;
          } else if constexpr (requires { value.conjugation; }) {
            analysis.which = value.conjugation.value;
            analysis.variant = value.conjugationVariant;
          }
        },
        dictionary);

  std::visit(
      [&](const auto& row) {
        template for (constexpr auto from : util::kMembers<std::remove_cvref_t<decltype(row)>>) {
          template for (constexpr auto to : util::kMembers<latin::Analysis>) {
            if constexpr (std::meta::identifier_of(from) == std::meta::identifier_of(to))
              analysis.[:to:] = row.[:from:];
          }
        }
      },
      inflection);

  switch (analysis.part) {
  case latin::Part::N:
    analysis.gender = std::get<latin::Noun>(dictionary).gender;
    break;
  case latin::Part::ADJ:
    analysis.comparison = expand::internal::adjectiveDegree(std::get<latin::Adjective>(dictionary), key);
    break;
  case latin::Part::NUM: {
    const latin::NumeralSort declared = std::get<latin::Numeral>(dictionary).numeralSort;
    analysis.numeralSort = declared == latin::NumeralSort::X ? numeralSortFromKey(key) : declared;
    break;
  }
  case latin::Part::ADV:
    if (!adverbFix) {
      const latin::Comparison declared = std::get<latin::Adverb>(dictionary).comparison;
      analysis.comparison = isDegree(declared) ? declared : advComparisonFromKey(key);
    }
    break;
  default:
    break;
  }
  return analysis;
}

[[nodiscard]] inline std::string describe(const latin::Analysis& analysis) {
  std::string text(latin::describe(analysis, nullptr, 0), '\0');
  latin::describe(analysis, text.data(), text.size() + 1);
  return text;
}

struct Rendered {
  std::string_view orth;
  std::string_view meaning;
  latin::Analysis analysis;
};

[[nodiscard]] inline Rendered render(const expand::Reading& reading) {
  const latin::Analysis analysis = analysisOf(reading.entry, reading.inflection, reading.key);
  return {reading.stem, reading.senses, analysis};
}

} // namespace emitter
