#pragma once

#include <array>
#include <cstddef>
#include <meta>
#include <ranges>
#include <variant>

#include "latin.hpp"
#include "types/grammar.hpp"

// INFO: Two grammars may join only where a `licenses` overload exists.
namespace expand::internal {

using namespace latin;

[[nodiscard]] constexpr bool licenses(const Noun& d, const inflected::Noun& i) noexcept {
  return declensionMatches(d.declension, d.declensionVariant, i.declension, i.declensionVariant) &&
         genderMatches(d.gender, i.gender);
}
[[nodiscard]] constexpr bool licenses(const Pronoun& d, const inflected::Pronoun& i) noexcept {
  return declensionMatches(d.declension, d.declensionVariant, i.declension, i.declensionVariant);
}
[[nodiscard]] constexpr bool licenses(const Adjective& d, const inflected::Adjective& i) noexcept {
  return declensionMatches(d.declension, d.declensionVariant, i.declension, i.declensionVariant) &&
         comparisonMatches(d.comparison, i.comparison);
}
[[nodiscard]] constexpr bool licenses(const Numeral& d, const inflected::Numeral& i) noexcept {
  return declensionMatches(d.declension, d.declensionVariant, i.declension, i.declensionVariant);
}
[[nodiscard]] constexpr bool licenses(const Adverb& d, const inflected::Adverb& i) noexcept {
  return comparisonMatches(d.comparison, i.comparison);
}
[[nodiscard]] constexpr bool licenses(const Verb& d, const inflected::Verb& i) noexcept {
  return conjugationMatches(d.conjugation, d.conjugationVariant, i.conjugation, i.conjugationVariant);
}
// INFO: word_package.adb:889 with Eff_Part (word_support_package.adb:29-38): VPAR and SUPINE rows join V entries.
[[nodiscard]] constexpr bool licenses(const Verb& d, const inflected::Participle& i) noexcept {
  return conjugationMatches(d.conjugation, d.conjugationVariant, i.conjugation, i.conjugationVariant);
}
[[nodiscard]] constexpr bool licenses(const Verb& d, const inflected::Supine& i) noexcept {
  return conjugationMatches(d.conjugation, d.conjugationVariant, i.conjugation, i.conjugationVariant);
}
[[nodiscard]] constexpr bool licenses(const Preposition& d, const inflected::Preposition& i) noexcept {
  return d.caseOf == i.caseOf;
}
[[nodiscard]] constexpr bool licenses(const Conjunction&, const inflected::Conjunction&) noexcept { return true; }
[[nodiscard]] constexpr bool licenses(const Interjection&, const inflected::Interjection&) noexcept { return true; }
// NOTE: A packon has no overload: it is half a word and joins a TACKON.

template <typename D, typename I>
concept MayPair = requires(const D& d, const I& i) { licenses(d, i); };

[[nodiscard]] constexpr bool licenses(const latin::Entry& dictionary, const latin::Inflection& inflection) noexcept {
  return std::visit(
      [](const auto& d, const auto& i) {
        if constexpr (MayPair<decltype(d), decltype(i)>)
          return licenses(d, i);
        else
          return false;
      },
      dictionary, inflection);
}

[[nodiscard]] constexpr bool packonLicenses(const latin::Entry& dictionary,
                                            const latin::Inflection& inflection) noexcept {
  const auto* packon = std::get_if<latin::Packon>(&dictionary);
  const auto* pronoun = std::get_if<latin::inflected::Pronoun>(&inflection);
  return packon != nullptr && pronoun != nullptr &&
         declensionMatches(packon->declension, packon->declensionVariant, pronoun->declension,
                           pronoun->declensionVariant);
}

inline constexpr std::size_t kGrammarCount = std::variant_size_v<latin::Entry>;
inline constexpr std::size_t kInflectionCount = std::variant_size_v<latin::Inflection>;

// INFO: The same rule by variant index, so the join can bucket inflections instead of crossing every row with every
//  row.
inline constexpr auto kPairs = [] {
  std::array<std::array<bool, kInflectionCount>, kGrammarCount> pairs{};
  template for (constexpr std::size_t d : std::define_static_array(std::views::iota(0uz, kGrammarCount))) {
    template for (constexpr std::size_t i : std::define_static_array(std::views::iota(0uz, kInflectionCount))) {
      pairs[d][i] =
          MayPair<std::variant_alternative_t<d, latin::Entry>, std::variant_alternative_t<i, latin::Inflection>>;
    }
  }
  return pairs;
}();

} // namespace expand::internal
