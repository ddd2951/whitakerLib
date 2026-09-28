#pragma once

#include "facts.hpp"
#include "expand/common/reading.hpp"
#include "expand/common/stem_column.hpp"
#include "latin.hpp"
#include "types/grammar.hpp"

#include <exception>
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

[[nodiscard]] inline latin::Comparison
advComparisonFromKey(latin::StemKey key) noexcept {
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

[[nodiscard]] inline latin::NumeralSort
numeralSortFromKey(latin::StemKey key) noexcept {
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
  return comparison == latin::Comparison::POS ||
         comparison == latin::Comparison::COMP ||
         comparison == latin::Comparison::SUPER;
}

[[nodiscard]] inline latin::Part partOf(const latin::Entry& grammar) noexcept {
  return std::visit(
      []<typename T>(const T&) {
        if constexpr (std::is_same_v<T, latin::Noun>)
          return latin::Part::N;
        else if constexpr (std::is_same_v<T, latin::Pronoun>)
          return latin::Part::PRON;
        else if constexpr (std::is_same_v<T, latin::Verb>)
          return latin::Part::V;
        else if constexpr (std::is_same_v<T, latin::Adjective>)
          return latin::Part::ADJ;
        else if constexpr (std::is_same_v<T, latin::Numeral>)
          return latin::Part::NUM;
        else if constexpr (std::is_same_v<T, latin::Packon>)
          return latin::Part::PACK;
        else if constexpr (std::is_same_v<T, latin::Adverb>)
          return latin::Part::ADV;
        else if constexpr (std::is_same_v<T, latin::Preposition>)
          return latin::Part::PREP;
        else if constexpr (std::is_same_v<T, latin::Conjunction>)
          return latin::Part::CONJ;
        else {
          static_assert(std::is_same_v<T, latin::Interjection>);
          return latin::Part::INTERJ;
        }
      },
      grammar);
}

[[nodiscard]] inline latin::Part
partOf(const latin::Inflection& grammar) noexcept {
  return std::visit(
      []<typename T>(const T&) {
        if constexpr (std::is_same_v<T, latin::inflected::Noun>)
          return latin::Part::N;
        else if constexpr (std::is_same_v<T, latin::inflected::Pronoun>)
          return latin::Part::PRON;
        else if constexpr (std::is_same_v<T, latin::inflected::Verb>)
          return latin::Part::V;
        else if constexpr (std::is_same_v<T, latin::inflected::Adjective>)
          return latin::Part::ADJ;
        else if constexpr (std::is_same_v<T, latin::inflected::Numeral>)
          return latin::Part::NUM;
        else if constexpr (std::is_same_v<T, latin::inflected::Participle>)
          return latin::Part::VPAR;
        else if constexpr (std::is_same_v<T, latin::inflected::Supine>)
          return latin::Part::SUPINE;
        else if constexpr (std::is_same_v<T, latin::inflected::Adverb>)
          return latin::Part::ADV;
        else if constexpr (std::is_same_v<T, latin::inflected::Preposition>)
          return latin::Part::PREP;
        else if constexpr (std::is_same_v<T, latin::inflected::Conjunction>)
          return latin::Part::CONJ;
        else {
          static_assert(std::is_same_v<T, latin::inflected::Interjection>);
          return latin::Part::INTERJ;
        }
      },
      grammar);
}

// INFO: Wildcard matches retain the dictionary's concrete grammar values.
[[nodiscard]] inline latin::Analysis
analysisOf(const latin::Entry& dictionary, const latin::Inflection& inflection,
           latin::StemKey key) {
  latin::Analysis analysis{.part = partOf(inflection)};
  const bool adverbFix = analysis.part == latin::Part::ADV &&
                         partOf(dictionary) != latin::Part::ADV;
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

  switch (analysis.part) {
  case latin::Part::N: {
    const auto& row = std::get<latin::inflected::Noun>(inflection);
    analysis.caseOf = row.caseOf;
    analysis.number = row.number;
    analysis.gender = std::get<latin::Noun>(dictionary).gender;
    break;
  }
  case latin::Part::PRON: {
    const auto& row = std::get<latin::inflected::Pronoun>(inflection);
    analysis.caseOf = row.caseOf;
    analysis.number = row.number;
    analysis.gender = row.gender;
    break;
  }
  case latin::Part::ADJ: {
    const auto& row = std::get<latin::inflected::Adjective>(inflection);
    analysis.caseOf = row.caseOf;
    analysis.number = row.number;
    analysis.gender = row.gender;
    analysis.comparison = expand::internal::adjectiveDegree(
        std::get<latin::Adjective>(dictionary), key);
    break;
  }
  case latin::Part::NUM: {
    const auto& row = std::get<latin::inflected::Numeral>(inflection);
    const latin::NumeralSort declared =
        std::get<latin::Numeral>(dictionary).numeralSort;
    analysis.caseOf = row.caseOf;
    analysis.number = row.number;
    analysis.gender = row.gender;
    analysis.numeralSort =
        declared == latin::NumeralSort::X ? numeralSortFromKey(key) : declared;
    break;
  }
  case latin::Part::ADV: {
    const auto& row = std::get<latin::inflected::Adverb>(inflection);
    if (adverbFix) {
      analysis.comparison = row.comparison;
      break;
    }
    const latin::Comparison declared =
        std::get<latin::Adverb>(dictionary).comparison;
    analysis.comparison =
        isDegree(declared) ? declared : advComparisonFromKey(key);
    break;
  }
  case latin::Part::V: {
    const auto& row = std::get<latin::inflected::Verb>(inflection);
    analysis.tense = row.tense;
    analysis.voice = row.voice;
    analysis.mood = row.mood;
    analysis.person = row.person;
    analysis.number = row.number;
    break;
  }
  case latin::Part::VPAR: {
    const auto& row = std::get<latin::inflected::Participle>(inflection);
    analysis.caseOf = row.caseOf;
    analysis.number = row.number;
    analysis.gender = row.gender;
    analysis.tense = row.tense;
    analysis.voice = row.voice;
    analysis.mood = row.mood;
    break;
  }
  case latin::Part::SUPINE: {
    const auto& row = std::get<latin::inflected::Supine>(inflection);
    analysis.caseOf = row.caseOf;
    analysis.number = row.number;
    analysis.gender = row.gender;
    break;
  }
  case latin::Part::PREP:
    analysis.caseOf =
        std::get<latin::inflected::Preposition>(inflection).caseOf;
    break;
  case latin::Part::CONJ:
  case latin::Part::INTERJ:
    break;
  default:
    std::terminate();
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
  const latin::Analysis analysis =
      analysisOf(reading.entry, reading.inflection, reading.key);
  return {reading.stem, reading.senses, analysis};
}

} // namespace emitter
