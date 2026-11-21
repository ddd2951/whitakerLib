#pragma once

#include "facts.hpp"
#include "src/search/two_ended_schema.hpp"
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

[[nodiscard]] inline std::string normalize(const std::string_view text) {
  std::string result{text};
  for (char& c : result)
    c = facts::foldLetter(c);
  return result;
}

[[nodiscard]] inline latin::Comparison advComparisonFromKey(const latin::StemKey key) {
  switch (key.value) {
    case 1:
      return latin::Comparison::positive;
    case 2:
      return latin::Comparison::comparative;
    case 3:
      return latin::Comparison::superlative;
    default:
      return latin::Comparison::any;
  }
}

[[nodiscard]] inline latin::NumeralSort numeralSortFromKey(const latin::StemKey key) {
  switch (key.value) {
    case 1:
      return latin::NumeralSort::cardinal;
    case 2:
      return latin::NumeralSort::ordinal;
    case 3:
      return latin::NumeralSort::distributive;
    case 4:
      return latin::NumeralSort::adverbial;
    default:
      return latin::NumeralSort::any;
  }
}

[[nodiscard]] inline bool isDegree(const latin::Comparison comparison) {
  return comparison == latin::Comparison::positive || comparison == latin::Comparison::comparative ||
         comparison == latin::Comparison::superlative;
}

using latin::partOf;

// INFO: Wildcard matches retain the dictionary's concrete grammar values.
[[nodiscard]] inline latin::Analysis analysisOf(const latin::Entry& dictionary, const latin::Inflection& inflection,
                                                const latin::StemKey key) {
  latin::Analysis analysis{.part = partOf(inflection)};
  const bool adverbFix = analysis.part == latin::Part::adverb && partOf(dictionary) != latin::Part::adverb;
  if (!adverbFix)
    std::visit(
        [&](const auto& value) {
          if constexpr (requires { value.declension; }) {
            analysis.which = {value.declension.value};
            analysis.variant = value.declensionVariant;
          } else if constexpr (requires { value.conjugation; }) {
            analysis.which = {value.conjugation.value};
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
    case latin::Part::noun:
      analysis.gender = std::get<latin::Noun>(dictionary).gender;
      break;
    case latin::Part::adjective:
      analysis.comparison = expand::internal::adjectiveDegree(std::get<latin::Adjective>(dictionary), key);
      break;
    case latin::Part::numeral: {
      const latin::NumeralSort declared = std::get<latin::Numeral>(dictionary).numeralSort;
      analysis.numeralSort = declared == latin::NumeralSort::any ? numeralSortFromKey(key) : declared;
      break;
    }
    case latin::Part::adverb:
      if (!adverbFix) {
        const latin::Comparison declared = std::get<latin::Adverb>(dictionary).comparison;
        analysis.comparison = isDegree(declared) ? declared : advComparisonFromKey(key);
      }
      break;
    case latin::Part::any:
    case latin::Part::pronoun:
    case latin::Part::verb:
    case latin::Part::preposition:
    case latin::Part::conjunction:
    case latin::Part::interjection:
    case latin::Part::packon:
    case latin::Part::supine:
    case latin::Part::participle:
    case latin::Part::none:
      break;
  }
  return analysis;
}

using whitaker::two_ended::printed;

[[nodiscard]] inline std::string describe(const latin::Analysis& analysis) {
  std::string text(latin::describe(analysis, nullptr, 0), '\0');
  latin::describe(analysis, text.data(), text.size() + 1);
  return text;
}

struct Rendered {
  std::string_view orth{};
  std::string_view meaning{};
  latin::Analysis analysis{};
};
static_assert(std::is_trivially_copyable_v<Rendered>);

[[nodiscard]] inline Rendered render(const expand::Reading& reading) {
  const latin::Analysis analysis = printed(analysisOf(reading.entry, reading.inflection, reading.key));
  return {reading.stem, reading.senses, analysis};
}

} // namespace emitter
