#pragma once

#include <meta>
#include <string_view>
#include <variant>

#include "latin.hpp"

namespace latin {

struct[[= Part::N]] Noun {
  Declension declension;
  Variant declensionVariant;
  Gender gender;
  NounKind nounKind;
};
struct[[= Part::PRON]] Pronoun {
  Declension declension;
  Variant declensionVariant;
  PronounKind pronounKind;
};
struct[[= Part::V]] Verb {
  Conjugation conjugation;
  Variant conjugationVariant;
  VerbKind verbKind;
};
struct[[= Part::ADJ]] Adjective {
  Declension declension;
  Variant declensionVariant;
  Comparison comparison;
};
struct[[= Part::NUM]] Numeral {
  Declension declension;
  Variant declensionVariant;
  NumeralSort numeralSort;
  NumeralValue numeralValue;
};
struct[[= Part::PACK]] Packon {
  Declension declension;
  Variant declensionVariant;
  PackonKind packonKind;
};
struct[[= Part::ADV]] Adverb {
  Comparison comparison;
};
struct[[= Part::PREP]] Preposition {
  Case caseOf;
};
struct[[= Part::CONJ]] Conjunction {};
struct[[= Part::INTERJ]] Interjection {};

using Entry =
    std::variant<Noun, Pronoun, Verb, Adjective, Numeral, Packon, Adverb, Preposition, Conjunction, Interjection>;

namespace inflected {
struct[[= Part::N]] Noun {
  Declension declension;
  Variant declensionVariant;
  Case caseOf;
  Number number;
  Gender gender;
};
struct[[= Part::PRON]] Pronoun {
  Declension declension;
  Variant declensionVariant;
  Case caseOf;
  Number number;
  Gender gender;
};
struct[[= Part::ADJ]] Adjective {
  Declension declension;
  Variant declensionVariant;
  Case caseOf;
  Number number;
  Gender gender;
  Comparison comparison;
};
struct[[= Part::NUM]] Numeral {
  Declension declension;
  Variant declensionVariant;
  Case caseOf;
  Number number;
  Gender gender;
  NumeralSort numeralSort;
};
struct[[= Part::V]] Verb {
  Conjugation conjugation;
  Variant conjugationVariant;
  Tense tense;
  Voice voice;
  Mood mood;
  Person person;
  Number number;
};
struct[[= Part::VPAR]] Participle {
  Conjugation conjugation;
  Variant conjugationVariant;
  Case caseOf;
  Number number;
  Gender gender;
  Tense tense;
  Voice voice;
  Mood mood;
};
struct[[= Part::SUPINE]] Supine {
  Conjugation conjugation;
  Variant conjugationVariant;
  Case caseOf;
  Number number;
  Gender gender;
};
struct[[= Part::ADV]] Adverb {
  Comparison comparison;
};
struct[[= Part::PREP]] Preposition {
  Case caseOf;
};
struct[[= Part::CONJ]] Conjunction {};
struct[[= Part::INTERJ]] Interjection {};
} // namespace inflected

using Inflection = std::variant<inflected::Noun, inflected::Pronoun, inflected::Adjective, inflected::Numeral,
                                inflected::Verb, inflected::Participle, inflected::Supine, inflected::Adverb,
                                inflected::Preposition, inflected::Conjunction, inflected::Interjection>;

namespace addon {
struct[[= Part::X]] Any {};

using Target = std::variant<Any, Noun, Pronoun, Adjective, Numeral, Verb, Adverb, Packon>;

// NOTE: prefix + suffix missing connection == '\0'
struct Prefix {
  std::string_view fix;
  char connect;
  Part from;
  Part to;
};
struct Suffix {
  std::string_view fix;
  char connect;
  Part from;
  StemKey fromKey;
  Part target;
  Target grammar;
  StemKey toKey;
};
struct Tackon {
  std::string_view fix;
  Part target;
  Target grammar;
};
} // namespace addon

using Addon = std::variant<addon::Prefix, addon::Suffix, addon::Tackon>;

template <typename T> consteval Part partTag() {
  return std::meta::extract<Part>(std::meta::annotations_of_with_type(^^T, ^^Part)[0]);
}

template <typename... Alternative>
[[nodiscard]] constexpr Part partOf(const std::variant<Alternative...>& grammar) noexcept {
  return std::visit([]<typename T>(const T&) { return partTag<T>(); }, grammar);
}

} // namespace latin
