#pragma once

#include <cstdint>
#include <string_view>

#include "source/wl_stable_schemes.hpp"
#include "types/domain.hpp"
#include "types/tokenized_sources.hpp"
#include "types/types.hpp"

namespace wl_stable::expand {

// INFO: One DICTLINE row as a value: what the tokenizer's columns hold at one
//  index, and what a synthetic row states.
struct DictlineEntry {
  tokenized::Stem stem1;
  tokenized::Stem stem2;
  tokenized::Stem stem3;
  tokenized::Stem stem4;
  tokenized::Grammar grammar;
  TypeAge age;
  TypeArea area;
  TypeGeography geography;
  TypeFrequency frequency;
  TypeSource source;
  std::string_view senses;
};

// INFO: One INFLECTS row as a value.
struct InflectsEntry {
  tokenized::Inflection grammar;
  domain::StemIndex stemKey;
  domain::CharacterCount characterCount;
  std::string_view ending;
  TypeAge age;
  TypeFrequency frequency;
};

// INFO: How many rows each source states.
inline constexpr std::uint32_t kDictlineRows{
    source::wl_stable::scheme::getEntryCount(tokenized::Kind::dictline)};
inline constexpr std::uint32_t kInflectsRows{
    source::wl_stable::scheme::getEntryCount(tokenized::Kind::inflects)};
inline constexpr std::uint32_t kUniquesRows{
    source::wl_stable::scheme::getEntryCount(tokenized::Kind::uniques)};

} // namespace wl_stable::expand
