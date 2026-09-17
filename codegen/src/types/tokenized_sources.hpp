#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <variant>

#include "source/wl_stable_schemes.hpp"
#include "types/domain.hpp"
#include "types/string_pools.hpp"
#include "types/types.hpp"

namespace tokenized {

using Kind = source::wl_stable::scheme::SourceFileKind;

template <Kind kind, typename T>
using Column = std::array<T, source::wl_stable::scheme::getEntryCount(kind)>;

// NOTE: A stem states one of the three following things
// Text
// zzz = absent
// all-blank = present-and-empty
using Stem = std::string_view;

enum class StemState : std::uint8_t { text, absent, empty };

[[nodiscard]] constexpr StemState stemState(Stem stem) {
  if (stem.empty())
    return StemState::empty;
  if (stem == text::StringPools::kAbsentStem)
    return StemState::absent;
  return StemState::text;
}

struct Noun {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypeGender gender;
  TypeNounKind nounKind;
};
struct Pronoun {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypePronounKind pronounKind;
};
struct Verb {
  domain::Conjugation conjugation;
  domain::ConjugationVariant conjugationVariant;
  TypeVerbKind verbKind;
};
struct Adjective {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypeComparison comparison;
};
struct Numeral {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypeNumeralSort numeralSort;
  domain::NumeralValue numeralValue;
};
struct Packon {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypePackonKind packonKind;
};
struct Adverb {
  TypeComparison comparison;
};
struct Preposition {
  TypeCase caseOf;
};
struct Conjunction {};
struct Interjection {};

using Grammar = std::variant<Noun, Pronoun, Verb, Adjective, Numeral, Packon,
                             Adverb, Preposition, Conjunction, Interjection>;

struct Dictline {
  template <typename T> using Col = Column<Kind::dictline, T>;

  Col<Stem> stem1;
  Col<Stem> stem2;
  Col<Stem> stem3;
  Col<Stem> stem4;

  Col<Grammar> grammar;

  Col<TypeAge> age;
  Col<TypeArea> area;
  Col<TypeGeography> geography;
  Col<TypeFrequency> frequency;
  Col<TypeSource> source;

  Col<std::string_view> senses;
};

namespace inflected {
struct Noun {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypeCase caseOf;
  TypeNumber number;
  TypeGender gender;
};
struct Pronoun {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypeCase caseOf;
  TypeNumber number;
  TypeGender gender;
};
struct Adjective {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypeCase caseOf;
  TypeNumber number;
  TypeGender gender;
  TypeComparison comparison;
};
struct Numeral {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypeCase caseOf;
  TypeNumber number;
  TypeGender gender;
  TypeNumeralSort numeralSort;
};
struct Verb {
  domain::Conjugation conjugation;
  domain::ConjugationVariant conjugationVariant;
  TypeTense tense;
  TypeVoice voice;
  TypeMood mood;
  domain::Person person;
  TypeNumber number;
};
struct Participle {
  domain::Conjugation conjugation;
  domain::ConjugationVariant conjugationVariant;
  TypeCase caseOf;
  TypeNumber number;
  TypeGender gender;
  TypeTense tense;
  TypeVoice voice;
  TypeMood mood;
};
struct Supine {
  domain::Conjugation conjugation;
  domain::ConjugationVariant conjugationVariant;
  TypeCase caseOf;
  TypeNumber number;
  TypeGender gender;
};
struct Adverb {
  TypeComparison comparison;
};
struct Preposition {
  TypeCase caseOf;
};
struct Conjunction {};
struct Interjection {};
} // namespace inflected

using Inflection =
    std::variant<inflected::Noun, inflected::Pronoun, inflected::Adjective,
                 inflected::Numeral, inflected::Verb, inflected::Participle,
                 inflected::Supine, inflected::Adverb, inflected::Preposition,
                 inflected::Conjunction, inflected::Interjection>;

struct Inflects {
  template <typename T> using Col = Column<Kind::inflects, T>;

  Col<Inflection> grammar;
  Col<domain::StemIndex> stemKey;
  Col<domain::CharacterCount> characterCount;
  Col<std::string_view> ending;
  Col<TypeAge> age;
  Col<TypeFrequency> frequency;
};

namespace unique {
struct Noun {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypeCase caseOf;
  TypeNumber number;
  TypeGender gender;
  TypeNounKind nounKind;
};
struct Pronoun {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypeCase caseOf;
  TypeNumber number;
  TypeGender gender;
  TypePronounKind pronounKind;
};
struct Adjective {
  domain::Declension declension;
  domain::DeclensionVariant declensionVariant;
  TypeCase caseOf;
  TypeNumber number;
  TypeGender gender;
  TypeComparison comparison;
};
struct Verb {
  domain::Conjugation conjugation;
  domain::ConjugationVariant conjugationVariant;
  TypeTense tense;
  TypeVoice voice;
  TypeMood mood;
  domain::Person person;
  TypeNumber number;
  TypeVerbKind verbKind;
};
} // namespace unique

using UniqueGrammar = std::variant<unique::Noun, unique::Pronoun,
                                   unique::Adjective, unique::Verb>;

struct Uniques {
  template <typename T> using Col = Column<Kind::uniques, T>;

  Col<std::string_view> form;
  Col<UniqueGrammar> grammar;
  Col<TypeAge> age;
  Col<TypeArea> area;
  Col<TypeGeography> geography;
  Col<TypeFrequency> frequency;
  Col<TypeSource> source;
  Col<std::string_view> senses;
};

namespace addon {
struct Any {};

// What an addon produces: a dictionary-shaped grammar, or any.
using Target =
    std::variant<Any, Noun, Pronoun, Adjective, Numeral, Verb, Adverb, Packon>;

struct Prefix {
  std::string_view fix;
  char connect; // '\0' when none
  PosTokenTypes from;
  PosTokenTypes to;
};
struct Suffix {
  std::string_view fix;
  char connect;
  PosTokenTypes from;
  domain::StemIndex fromKey;
  PosTokenTypes target;
  Target grammar;
  domain::StemIndex toKey;
};
struct Tackon {
  std::string_view fix;
  PosTokenTypes target;
  Target grammar;
};
} // namespace addon

using Addon = std::variant<addon::Prefix, addon::Suffix, addon::Tackon>;

struct Addons {
  template <typename T> using Col = Column<Kind::addons, T>;

  Col<Addon> addon;
  Col<std::string_view> meaning;
};

struct Sources {
  Dictline dictline;
  Inflects inflects;
  Uniques uniques;
  Addons addons;
};

} // namespace tokenized
