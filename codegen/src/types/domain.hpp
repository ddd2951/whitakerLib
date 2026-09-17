#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "shared/semantic/value.hpp"

namespace domain {

struct DeclensionTag;
struct DeclensionVariantTag;
struct ConjugationTag;
struct ConjugationVariantTag;
struct StemIndexTag;
struct CharacterCountTag;
struct PersonTag;
struct NumeralValueTag;

using Declension = semantic::Value<DeclensionTag, std::uint8_t>;
using DeclensionVariant = semantic::Value<DeclensionVariantTag, std::uint8_t>;
using Conjugation = semantic::Value<ConjugationTag, std::uint8_t>;
using ConjugationVariant = semantic::Value<ConjugationVariantTag, std::uint8_t>;
using StemIndex = semantic::Value<StemIndexTag, std::uint8_t>;
using CharacterCount = semantic::Value<CharacterCountTag, std::uint8_t>;
using Person = semantic::Value<PersonTag, std::uint8_t>;
using NumeralValue = semantic::Value<NumeralValueTag, std::uint16_t>;

// INFO: Declensions 7 and 8 are not part of the source domain.
[[nodiscard]] constexpr bool isValid(Declension d) noexcept {
  return d.value <= 6 || d.value == 9;
}
[[nodiscard]] constexpr bool isValid(DeclensionVariant v) noexcept {
  return v.value <= 9;
}
[[nodiscard]] constexpr bool isValid(Conjugation c) noexcept {
  return c.value <= 9;
}
[[nodiscard]] constexpr bool isValid(ConjugationVariant v) noexcept {
  return v.value <= 9;
}
[[nodiscard]] constexpr bool isValid(StemIndex key) noexcept {
  return key.value <= 9;
}
[[nodiscard]] constexpr bool isValid(CharacterCount count) noexcept {
  return count.value <= 7;
}
[[nodiscard]] constexpr bool isValid(Person person) noexcept {
  return person.value <= 3;
}
// INFO: Whitaker's NUMERAL_VALUE_TYPE is range 0..1000.
[[nodiscard]] constexpr bool isValid(NumeralValue value) noexcept {
  return value.value <= 1000;
}

namespace detail {
inline constexpr std::string_view kDeclensionNames[]{
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9"};
inline constexpr std::string_view kVariantNames[]{"V0", "V1", "V2", "V3", "V4",
                                                  "V5", "V6", "V7", "V8", "V9"};
inline constexpr std::string_view kConjugationNames[]{
    "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9"};
} // namespace detail

[[nodiscard]] constexpr std::string_view name(Declension d) noexcept {
  return isValid(d) ? detail::kDeclensionNames[d.value] : "?";
}
[[nodiscard]] constexpr std::string_view name(DeclensionVariant v) noexcept {
  return isValid(v) ? detail::kVariantNames[v.value] : "?";
}
[[nodiscard]] constexpr std::string_view name(Conjugation c) noexcept {
  return isValid(c) ? detail::kConjugationNames[c.value] : "?";
}
[[nodiscard]] constexpr std::string_view name(ConjugationVariant v) noexcept {
  return isValid(v) ? detail::kVariantNames[v.value] : "?";
}

} // namespace domain
