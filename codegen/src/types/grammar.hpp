#pragma once

#include <string_view>
#include <variant>

#include "latin.hpp"

namespace latin {

struct Noun {
  Declension declension;
  Variant declensionVariant;
  Gender gender;
  NounKind nounKind;
};
struct Pronoun {
  Declension declension;
  Variant declensionVariant;
  PronounKind pronounKind;
};
struct Verb {
  Conjugation conjugation;
  Variant conjugationVariant;
  VerbKind verbKind;
};
struct Adjective {
  Declension declension;
  Variant declensionVariant;
  Comparison comparison;
};
struct Numeral {
  Declension declension;
  Variant declensionVariant;
  NumeralSort numeralSort;
  NumeralValue numeralValue;
};
struct Packon {
  Declension declension;
  Variant declensionVariant;
  PackonKind packonKind;
};
struct Adverb {
  Comparison comparison;
};
struct Preposition {
  Case caseOf;
};
struct Conjunction {};
struct Interjection {};

using Entry = std::variant<Noun, Pronoun, Verb, Adjective, Numeral, Packon,
                           Adverb, Preposition, Conjunction, Interjection>;

namespace inflected {
struct Noun {
  Declension declension;
  Variant declensionVariant;
  Case caseOf;
  Number number;
  Gender gender;
};
struct Pronoun {
  Declension declension;
  Variant declensionVariant;
  Case caseOf;
  Number number;
  Gender gender;
};
struct Adjective {
  Declension declension;
  Variant declensionVariant;
  Case caseOf;
  Number number;
  Gender gender;
  Comparison comparison;
};
struct Numeral {
  Declension declension;
  Variant declensionVariant;
  Case caseOf;
  Number number;
  Gender gender;
  NumeralSort numeralSort;
};
struct Verb {
  Conjugation conjugation;
  Variant conjugationVariant;
  Tense tense;
  Voice voice;
  Mood mood;
  Person person;
  Number number;
};
struct Participle {
  Conjugation conjugation;
  Variant conjugationVariant;
  Case caseOf;
  Number number;
  Gender gender;
  Tense tense;
  Voice voice;
  Mood mood;
};
struct Supine {
  Conjugation conjugation;
  Variant conjugationVariant;
  Case caseOf;
  Number number;
  Gender gender;
};
struct Adverb {
  Comparison comparison;
};
struct Preposition {
  Case caseOf;
};
struct Conjunction {};
struct Interjection {};
} // namespace inflected

using Inflection =
    std::variant<inflected::Noun, inflected::Pronoun, inflected::Adjective,
                 inflected::Numeral, inflected::Verb, inflected::Participle,
                 inflected::Supine, inflected::Adverb, inflected::Preposition,
                 inflected::Conjunction, inflected::Interjection>;

namespace addon {
struct Any {};

// What an addon produces: a dictionary-shaped grammar, or any.
using Target =
    std::variant<Any, Noun, Pronoun, Adjective, Numeral, Verb, Adverb, Packon>;

// NOTE: A missing connecting letter is '\0' for both prefix and suffix.
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

} // namespace latin
