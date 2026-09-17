#pragma once

#include "gen/expand/wl_stable/common/reading.hpp"
#include "gen/expand/wl_stable/common/stem_column.hpp"
#include "types/domain.hpp"
#include "types/tokenized_sources.hpp"
#include "types/types.hpp"

#include <exception>
#include <format>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace wl_stable::emitter::detail::rendered_analysis {

inline void append(std::string& text, std::string_view part) {
  if (part.empty() || part == "NONE")
    return;
  if (!text.empty())
    text += ' ';
  text += part;
}

[[nodiscard]] inline TypeComparison
advComparisonFromKey(domain::StemIndex key) noexcept {
  switch (key.value) {
  case 1:
    return TypeComparison::POS;
  case 2:
    return TypeComparison::COMP;
  case 3:
    return TypeComparison::SUPER;
  default:
    return TypeComparison::X;
  }
}

[[nodiscard]] inline TypeNumeralSort
numeralSortFromKey(domain::StemIndex key) noexcept {
  switch (key.value) {
  case 1:
    return TypeNumeralSort::CARD;
  case 2:
    return TypeNumeralSort::ORD;
  case 3:
    return TypeNumeralSort::DIST;
  case 4:
    return TypeNumeralSort::ADVERB;
  default:
    return TypeNumeralSort::X;
  }
}

[[nodiscard]] inline bool isDegree(TypeComparison comparison) noexcept {
  return comparison == TypeComparison::POS ||
         comparison == TypeComparison::COMP ||
         comparison == TypeComparison::SUPER;
}

[[nodiscard]] inline PosTokenTypes
partOf(const tokenized::Grammar& grammar) noexcept {
  return std::visit(
      []<typename T>(const T&) {
        if constexpr (std::is_same_v<T, tokenized::Noun>)
          return PosTokenTypes::N;
        else if constexpr (std::is_same_v<T, tokenized::Pronoun>)
          return PosTokenTypes::PRON;
        else if constexpr (std::is_same_v<T, tokenized::Verb>)
          return PosTokenTypes::V;
        else if constexpr (std::is_same_v<T, tokenized::Adjective>)
          return PosTokenTypes::ADJ;
        else if constexpr (std::is_same_v<T, tokenized::Numeral>)
          return PosTokenTypes::NUM;
        else if constexpr (std::is_same_v<T, tokenized::Packon>)
          return PosTokenTypes::PACK;
        else if constexpr (std::is_same_v<T, tokenized::Adverb>)
          return PosTokenTypes::ADV;
        else if constexpr (std::is_same_v<T, tokenized::Preposition>)
          return PosTokenTypes::PREP;
        else if constexpr (std::is_same_v<T, tokenized::Conjunction>)
          return PosTokenTypes::CONJ;
        else {
          static_assert(std::is_same_v<T, tokenized::Interjection>);
          return PosTokenTypes::INTERJ;
        }
      },
      grammar);
}

[[nodiscard]] inline PosTokenTypes
partOf(const tokenized::Inflection& grammar) noexcept {
  return std::visit(
      []<typename T>(const T&) {
        if constexpr (std::is_same_v<T, tokenized::inflected::Noun>)
          return PosTokenTypes::N;
        else if constexpr (std::is_same_v<T, tokenized::inflected::Pronoun>)
          return PosTokenTypes::PRON;
        else if constexpr (std::is_same_v<T, tokenized::inflected::Verb>)
          return PosTokenTypes::V;
        else if constexpr (std::is_same_v<T, tokenized::inflected::Adjective>)
          return PosTokenTypes::ADJ;
        else if constexpr (std::is_same_v<T, tokenized::inflected::Numeral>)
          return PosTokenTypes::NUM;
        else if constexpr (std::is_same_v<T, tokenized::inflected::Participle>)
          return PosTokenTypes::VPAR;
        else if constexpr (std::is_same_v<T, tokenized::inflected::Supine>)
          return PosTokenTypes::SUPINE;
        else if constexpr (std::is_same_v<T, tokenized::inflected::Adverb>)
          return PosTokenTypes::ADV;
        else if constexpr (std::is_same_v<T, tokenized::inflected::Preposition>)
          return PosTokenTypes::PREP;
        else if constexpr (std::is_same_v<T, tokenized::inflected::Conjunction>)
          return PosTokenTypes::CONJ;
        else {
          static_assert(std::is_same_v<T, tokenized::inflected::Interjection>);
          return PosTokenTypes::INTERJ;
        }
      },
      grammar);
}

inline void appendDictionaryNumber(std::string& text,
                                   const tokenized::Grammar& grammar) {
  std::visit(
      [&](const auto& value) {
        if constexpr (requires { value.declension; }) {
          append(text, domain::name(value.declension));
          append(text, domain::name(value.declensionVariant));
        } else if constexpr (requires { value.conjugation; }) {
          append(text, domain::name(value.conjugation));
          append(text, domain::name(value.conjugationVariant));
        }
      },
      grammar);
}

// INFO:  Wildcard matches retain the dictionary's concrete grammar values.
[[nodiscard]] inline std::string
describeAnalysis(const tokenized::Grammar& dictionary,
                 const tokenized::Inflection& inflection,
                 domain::StemIndex key) {
  const PosTokenTypes dictionaryPart = partOf(dictionary);
  const PosTokenTypes inflectionPart = partOf(inflection);
  const bool adverbFix = inflectionPart == PosTokenTypes::ADV &&
                         dictionaryPart != PosTokenTypes::ADV;

  std::string text{toName(inflectionPart)};
  if (!adverbFix)
    appendDictionaryNumber(text, dictionary);

  switch (inflectionPart) {
  case PosTokenTypes::N: {
    const auto& row = std::get<tokenized::inflected::Noun>(inflection);
    append(text, toName(row.caseOf));
    append(text, toName(row.number));
    append(text, toName(std::get<tokenized::Noun>(dictionary).gender));
    break;
  }
  case PosTokenTypes::PRON: {
    const auto& row = std::get<tokenized::inflected::Pronoun>(inflection);
    append(text, toName(row.caseOf));
    append(text, toName(row.number));
    append(text, toName(row.gender));
    break;
  }
  case PosTokenTypes::ADJ: {
    const auto& row = std::get<tokenized::inflected::Adjective>(inflection);
    append(text, toName(row.caseOf));
    append(text, toName(row.number));
    append(text, toName(row.gender));
    append(text, toName(wl_stable::expand::internal::adjectiveDegree(
                     std::get<tokenized::Adjective>(dictionary), key)));
    break;
  }
  case PosTokenTypes::NUM: {
    const auto& row = std::get<tokenized::inflected::Numeral>(inflection);
    const TypeNumeralSort declared =
        std::get<tokenized::Numeral>(dictionary).numeralSort;
    append(text, toName(row.caseOf));
    append(text, toName(row.number));
    append(text, toName(row.gender));
    append(text, toName(declared == TypeNumeralSort::X ? numeralSortFromKey(key)
                                                       : declared));
    break;
  }
  case PosTokenTypes::ADV: {
    const auto& row = std::get<tokenized::inflected::Adverb>(inflection);
    if (adverbFix) {
      append(text, toName(row.comparison));
      break;
    }
    const TypeComparison declared =
        std::get<tokenized::Adverb>(dictionary).comparison;
    append(text,
           toName(isDegree(declared) ? declared : advComparisonFromKey(key)));
    break;
  }
  case PosTokenTypes::V: {
    const auto& row = std::get<tokenized::inflected::Verb>(inflection);
    append(text, toName(row.tense));
    append(text, toName(row.voice));
    append(text, toName(row.mood));
    if (row.person.value != 0)
      append(text, std::format("{}", row.person.value));
    append(text, toName(row.number));
    break;
  }
  case PosTokenTypes::VPAR: {
    const auto& row = std::get<tokenized::inflected::Participle>(inflection);
    append(text, toName(row.caseOf));
    append(text, toName(row.number));
    append(text, toName(row.gender));
    append(text, toName(row.tense));
    append(text, toName(row.voice));
    append(text, toName(row.mood));
    break;
  }
  case PosTokenTypes::SUPINE: {
    const auto& row = std::get<tokenized::inflected::Supine>(inflection);
    append(text, toName(row.caseOf));
    append(text, toName(row.number));
    append(text, toName(row.gender));
    break;
  }
  case PosTokenTypes::PREP:
    append(
        text,
        toName(std::get<tokenized::inflected::Preposition>(inflection).caseOf));
    break;
  case PosTokenTypes::CONJ:
  case PosTokenTypes::INTERJ:
    break;
  default:
    std::terminate();
  }
  return text;
}

struct Rendered {
  std::string_view orth;
  std::string_view meaning;
  std::string_view pos;
  std::string inflection;
};

[[nodiscard]] inline Rendered
render(const wl_stable::expand::Reading& reading) {
  return {reading.stem, reading.senses, toName(partOf(reading.inflection)),
          describeAnalysis(reading.entry, reading.inflection, reading.key)};
}

} // namespace wl_stable::emitter::detail::rendered_analysis
