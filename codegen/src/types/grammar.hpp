#pragma once

#include <meta>
#include <string_view>
#include <variant>
#include <type_traits>

#include "latin.hpp"

namespace latin {

struct[[= Part::noun]] Noun {
  Declension declension{};
  Variant declensionVariant{};
  Gender gender{};
  NounKind nounKind{};
};
static_assert(std::is_trivially_copyable_v<Noun>);
struct[[= Part::pronoun]] Pronoun {
  Declension declension{};
  Variant declensionVariant{};
  PronounKind pronounKind{};
};
static_assert(std::is_trivially_copyable_v<Pronoun>);
struct[[= Part::verb]] Verb {
  Conjugation conjugation{};
  Variant conjugationVariant{};
  VerbKind verbKind{};
};
static_assert(std::is_trivially_copyable_v<Verb>);
struct[[= Part::adjective]] Adjective {
  Declension declension{};
  Variant declensionVariant{};
  Comparison comparison{};
};
static_assert(std::is_trivially_copyable_v<Adjective>);
struct[[= Part::numeral]] Numeral {
  Declension declension{};
  Variant declensionVariant{};
  NumeralSort numeralSort{};
  EntryNumeralValue numeralValue{};
};
static_assert(std::is_trivially_copyable_v<Numeral>);
struct[[= Part::packon]] Packon {
  Declension declension{};
  Variant declensionVariant{};
  PackonKind packonKind{};
};
static_assert(std::is_trivially_copyable_v<Packon>);
struct[[= Part::adverb]] Adverb {
  Comparison comparison{};
};
static_assert(std::is_trivially_copyable_v<Adverb>);
struct[[= Part::preposition]] Preposition {
  Case caseOf{};
};
static_assert(std::is_trivially_copyable_v<Preposition>);
struct[[= Part::conjunction]] Conjunction {};
static_assert(std::is_trivially_copyable_v<Conjunction>);
struct[[= Part::interjection]] Interjection {};
static_assert(std::is_trivially_copyable_v<Interjection>);

using Entry =
    std::variant<Noun, Pronoun, Verb, Adjective, Numeral, Packon, Adverb, Preposition, Conjunction, Interjection>;
static_assert(std::is_trivially_copyable_v<Entry>);

namespace inflected {
struct[[= Part::noun]] Noun {
  Declension declension{};
  Variant declensionVariant{};
  Case caseOf{};
  Number number{};
  Gender gender{};
};
static_assert(std::is_trivially_copyable_v<Noun>);
struct[[= Part::pronoun]] Pronoun {
  Declension declension{};
  Variant declensionVariant{};
  Case caseOf{};
  Number number{};
  Gender gender{};
};
static_assert(std::is_trivially_copyable_v<Pronoun>);
struct[[= Part::adjective]] Adjective {
  Declension declension{};
  Variant declensionVariant{};
  Case caseOf{};
  Number number{};
  Gender gender{};
  Comparison comparison{};
};
static_assert(std::is_trivially_copyable_v<Adjective>);
struct[[= Part::numeral]] Numeral {
  Declension declension{};
  Variant declensionVariant{};
  Case caseOf{};
  Number number{};
  Gender gender{};
  NumeralSort numeralSort{};
};
static_assert(std::is_trivially_copyable_v<Numeral>);
struct[[= Part::verb]] Verb {
  Conjugation conjugation{};
  Variant conjugationVariant{};
  Tense tense{};
  Voice voice{};
  Mood mood{};
  Person person{};
  Number number{};
};
static_assert(std::is_trivially_copyable_v<Verb>);
struct[[= Part::participle]] Participle {
  Conjugation conjugation{};
  Variant conjugationVariant{};
  Case caseOf{};
  Number number{};
  Gender gender{};
  Tense tense{};
  Voice voice{};
  Mood mood{};
};
static_assert(std::is_trivially_copyable_v<Participle>);
struct[[= Part::supine]] Supine {
  Conjugation conjugation{};
  Variant conjugationVariant{};
  Case caseOf{};
  Number number{};
  Gender gender{};
};
static_assert(std::is_trivially_copyable_v<Supine>);
struct[[= Part::adverb]] Adverb {
  Comparison comparison{};
};
static_assert(std::is_trivially_copyable_v<Adverb>);
struct[[= Part::preposition]] Preposition {
  Case caseOf{};
};
static_assert(std::is_trivially_copyable_v<Preposition>);
struct[[= Part::conjunction]] Conjunction {};
static_assert(std::is_trivially_copyable_v<Conjunction>);
struct[[= Part::interjection]] Interjection {};
static_assert(std::is_trivially_copyable_v<Interjection>);
} // namespace inflected

using Inflection = std::variant<inflected::Noun, inflected::Pronoun, inflected::Adjective, inflected::Numeral,
                                inflected::Verb, inflected::Participle, inflected::Supine, inflected::Adverb,
                                inflected::Preposition, inflected::Conjunction, inflected::Interjection>;
static_assert(std::is_trivially_copyable_v<Inflection>);

namespace addon {
struct[[= Part::any]] Any {};
static_assert(std::is_trivially_copyable_v<Any>);

using Target = std::variant<Any, Noun, Pronoun, Adjective, Numeral, Verb, Adverb, Packon>;
static_assert(std::is_trivially_copyable_v<Target>);

// NOTE: prefix + suffix missing connection == '\0'
struct Prefix {
  std::string_view fix{};
  char connect{};
  Part from{};
  Part to{};
};
static_assert(std::is_trivially_copyable_v<Prefix>);
struct Suffix {
  std::string_view fix{};
  char connect{};
  Part from{};
  StemKey fromKey{};
  Part target{};
  Target grammar{};
  StemKey toKey{};
};
static_assert(std::is_trivially_copyable_v<Suffix>);
struct Tackon {
  std::string_view fix{};
  Part target{};
  Target grammar{};
};
static_assert(std::is_trivially_copyable_v<Tackon>);
} // namespace addon

using Addon = std::variant<addon::Prefix, addon::Suffix, addon::Tackon>;
static_assert(std::is_trivially_copyable_v<Addon>);

template <typename T> consteval Part partTag() {
  return std::meta::extract<Part>(std::meta::annotations_of_with_type(^^T, ^^Part)[0]);
}

template <typename... Alternative> [[nodiscard]] constexpr Part partOf(const std::variant<Alternative...>& grammar) {
  return std::visit([]<typename T>(const T&) { return partTag<T>(); }, grammar);
}

} // namespace latin
