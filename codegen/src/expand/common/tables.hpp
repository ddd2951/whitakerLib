#pragma once

#include <cstdint>
#include <string_view>

#include "source/schemes.hpp"
#include "latin.hpp"
#include "types/grammar.hpp"
#include "word/word.hpp"

namespace expand {

struct DictlineEntry {
  word::Stem stem1;
  word::Stem stem2;
  word::Stem stem3;
  word::Stem stem4;
  latin::Entry grammar;
  latin::Age age;
  latin::Area area;
  latin::Geography geography;
  latin::Frequency frequency;
  latin::Source source;
  std::string_view senses;
};

struct InflectsEntry {
  latin::Inflection grammar;
  latin::StemKey stemKey;
  latin::CharacterCount characterCount;
  std::string_view ending;
  latin::Age age;
  latin::Frequency frequency;
};

inline constexpr std::uint32_t kDictlineRows{
    source::scheme::getEntryCount(source::scheme::SourceFileKind::dictline)};
inline constexpr std::uint32_t kInflectsRows{
    source::scheme::getEntryCount(source::scheme::SourceFileKind::inflects)};
inline constexpr std::uint32_t kUniquesRows{
    source::scheme::getEntryCount(source::scheme::SourceFileKind::uniques)};

} // namespace expand
