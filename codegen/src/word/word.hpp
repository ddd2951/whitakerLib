#pragma once

#include "source/source.hpp"
#include "types/grammar.hpp"
#include "latin.hpp"

#include <cstddef>
#include <cstdint>
#include <meta>
#include <optional>
#include <string_view>

namespace word {

using Stem = std::optional<std::string_view>;

template <std::meta::info file> struct Row {
  std::uint32_t value{};
  friend constexpr bool operator==(Row, Row) = default;
};

using DictlineRow = Row<^^source::scheme::dictline>;
using InflectsRow = Row<^^source::scheme::inflects>;
using UniquesRow = Row<^^source::scheme::uniques>;

inline constexpr DictlineRow kEsse{source::scheme::dictline::kEntriesPerFile};
inline constexpr InflectsRow kAdverbPositive{source::scheme::inflects::kEntriesPerFile};
inline constexpr InflectsRow kAdverbSuperlative{kAdverbPositive.value + 1};

inline constexpr std::uint32_t kDictlineWords{kEsse.value + 1};
inline constexpr std::uint32_t kInflectsWords{kAdverbSuperlative.value + 1};

void init();

struct Column {
  std::size_t at;
  std::size_t width;
};

struct PartColumn {
  std::size_t at;
  std::size_t width;
};

struct PresentIf {
  char member[24]{};
  consteval PresentIf(const char* name) {
    for (std::size_t i = 0; name[i] != '\0'; ++i)
      member[i] = name[i];
  }
};

struct Labels {
  [[= Column{1, 1}]] latin::Age age;
  [[= Column{3, 1}]] latin::Area area;
  [[= Column{5, 1}]] latin::Geography geography;
  [[= Column{7, 1}]] latin::Frequency frequency;
  [[= Column{9, 1}]] latin::Source source;
};

struct DictlineEntry {
  [[= Column{0, 19}]] Stem stem1;
  [[= Column{19, 19}]] Stem stem2;
  [[= Column{38, 19}]] Stem stem3;
  [[= Column{57, 19}]] Stem stem4;
  [[ = Column{83, 16}, = PartColumn{76, 7} ]] latin::Entry grammar;
  [[= Column{99, 11}]] Labels labels;
  [[= Column{110, 0}]] std::string_view senses;
  std::string_view packon;
};

struct InflectsEntry {
  latin::Inflection grammar;
  latin::StemKey stemKey;
  latin::CharacterCount characterCount;
  [[= PresentIf{"characterCount"}]] std::string_view ending;
  latin::Age age;
  latin::Frequency frequency;
};

struct UniquesEntry {
  std::string_view form;
  latin::Entry grammar;
  latin::Inflection inflection;
  Labels labels;
  std::string_view senses;
};

struct AddonsEntry {
  latin::Addon addon;
  std::string_view meaning;
  latin::AddonKind kind;
};

[[nodiscard]] const DictlineEntry& entry(DictlineRow row);
[[nodiscard]] const InflectsEntry& entry(InflectsRow row);
[[nodiscard]] const UniquesEntry& entry(UniquesRow row);
[[nodiscard]] const AddonsEntry& entry(source::AddonsIndex row);

} // namespace word
