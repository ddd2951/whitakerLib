#pragma once

#include <cstddef>
#include <cstdint>
#include <variant>

#include "types/domain.hpp"
#include "types/tokenized_sources.hpp"
#include "types/types.hpp"

// INFO: Whitaker's matching predicates, dictionary on the left, inflection on
// the right. Two grammars may join only where a `licenses` overload exists.
namespace wl_stable::expand::internal {

[[nodiscard]] constexpr bool numberMatches(std::uint8_t left,
                                           std::uint8_t leftVariant,
                                           std::uint8_t right,
                                           std::uint8_t rightVariant) noexcept {
  if (right == left && rightVariant == leftVariant)
    return true;
  if (right == 0 && rightVariant == 0 && left != 9)
    return true;
  return right == left && rightVariant == 0;
}

[[nodiscard]] constexpr bool declensionMatches(
    domain::Declension left, domain::DeclensionVariant leftVariant,
    domain::Declension right, domain::DeclensionVariant rightVariant) noexcept {
  return numberMatches(left.value, leftVariant.value, right.value,
                       rightVariant.value);
}

[[nodiscard]] constexpr bool
conjugationMatches(domain::Conjugation left,
                   domain::ConjugationVariant leftVariant,
                   domain::Conjugation right,
                   domain::ConjugationVariant rightVariant) noexcept {
  return numberMatches(left.value, leftVariant.value, right.value,
                       rightVariant.value);
}

[[nodiscard]] constexpr bool genderMatches(TypeGender left,
                                           TypeGender right) noexcept {
  return right == left || right == TypeGender::X ||
         (right == TypeGender::C && left != TypeGender::N);
}

[[nodiscard]] constexpr bool comparisonMatches(TypeComparison left,
                                               TypeComparison right) noexcept {
  return right == left || right == TypeComparison::X ||
         left == TypeComparison::X;
}

using namespace tokenized;

[[nodiscard]] constexpr bool licenses(const Noun& d,
                                      const inflected::Noun& i) noexcept {
  return declensionMatches(d.declension, d.declensionVariant, i.declension,
                           i.declensionVariant) &&
         genderMatches(d.gender, i.gender);
}
[[nodiscard]] constexpr bool licenses(const Pronoun& d,
                                      const inflected::Pronoun& i) noexcept {
  return declensionMatches(d.declension, d.declensionVariant, i.declension,
                           i.declensionVariant);
}
[[nodiscard]] constexpr bool licenses(const Adjective& d,
                                      const inflected::Adjective& i) noexcept {
  return declensionMatches(d.declension, d.declensionVariant, i.declension,
                           i.declensionVariant) &&
         comparisonMatches(d.comparison, i.comparison);
}
[[nodiscard]] constexpr bool licenses(const Numeral& d,
                                      const inflected::Numeral& i) noexcept {
  return declensionMatches(d.declension, d.declensionVariant, i.declension,
                           i.declensionVariant);
}
[[nodiscard]] constexpr bool licenses(const Adverb& d,
                                      const inflected::Adverb& i) noexcept {
  return comparisonMatches(d.comparison, i.comparison);
}
[[nodiscard]] constexpr bool licenses(const Verb& d,
                                      const inflected::Verb& i) noexcept {
  return conjugationMatches(d.conjugation, d.conjugationVariant, i.conjugation,
                            i.conjugationVariant);
}
[[nodiscard]] constexpr bool licenses(const Verb& d,
                                      const inflected::Participle& i) noexcept {
  return conjugationMatches(d.conjugation, d.conjugationVariant, i.conjugation,
                            i.conjugationVariant);
}
[[nodiscard]] constexpr bool licenses(const Verb& d,
                                      const inflected::Supine& i) noexcept {
  return conjugationMatches(d.conjugation, d.conjugationVariant, i.conjugation,
                            i.conjugationVariant);
}
[[nodiscard]] constexpr bool
licenses(const Preposition& d, const inflected::Preposition& i) noexcept {
  return d.caseOf == i.caseOf;
}
[[nodiscard]] constexpr bool licenses(const Conjunction&,
                                      const inflected::Conjunction&) noexcept {
  return true;
}
[[nodiscard]] constexpr bool licenses(const Interjection&,
                                      const inflected::Interjection&) noexcept {
  return true;
}
// NOTE: A packon has no overload: it is half a word and joins a TACKON.

template <typename D, typename I>
concept MayPair = requires(const D& d, const I& i) { licenses(d, i); };

[[nodiscard]] constexpr bool licenses(const Grammar& dictionary,
                                      const Inflection& inflection) noexcept {
  return std::visit(
      [](const auto& d, const auto& i) {
        if constexpr (MayPair<decltype(d), decltype(i)>)
          return licenses(d, i);
        else
          return false;
      },
      dictionary, inflection);
}

inline constexpr std::size_t kGrammarCount = std::variant_size_v<Grammar>;
inline constexpr std::size_t kInflectionCount = std::variant_size_v<Inflection>;

// INFO: The same rule by variant index, so the join can bucket inflections
// instead of crossing every row with every row.
template <std::size_t D, std::size_t I>
inline constexpr bool kMayPair =
    MayPair<std::variant_alternative_t<D, Grammar>,
            std::variant_alternative_t<I, Inflection>>;

} // namespace wl_stable::expand::internal
