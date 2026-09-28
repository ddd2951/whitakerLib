#pragma once

#include "source/source.hpp"
#include "types/grammar.hpp"
#include "latin.hpp"

#include <cstdint>
#include <string_view>

namespace word {

// NOTE: A stem is text, absent (`zzz`) or present and empty (all blanks).
using Stem = std::string_view;

inline constexpr std::string_view kAbsentStem{"zzz"};

enum class StemState : std::uint8_t { text, absent, empty };

[[nodiscard]] constexpr StemState stemState(Stem stem) {
  if (stem.empty())
    return StemState::empty;
  if (stem == kAbsentStem)
    return StemState::absent;
  return StemState::text;
}

struct DictlineRowTag;
struct InflectsRowTag;

using DictlineRow = semantic::Value<DictlineRowTag, std::uint32_t>;
using InflectsRow = semantic::Value<InflectsRowTag, std::uint32_t>;

inline constexpr DictlineRow kEsse{source::scheme::dictline::kEntriesPerFile};
inline constexpr InflectsRow kAdverbPositive{
    source::scheme::inflects::kEntriesPerFile};
inline constexpr InflectsRow kAdverbSuperlative{kAdverbPositive.value + 1};

inline constexpr std::uint32_t kDictlineWords{kEsse.value + 1};
inline constexpr std::uint32_t kInflectsWords{kAdverbSuperlative.value + 1};

void init();

class Dictline {
public:
  explicit constexpr Dictline(DictlineRow entry) : entry{entry} {}

  [[nodiscard]] word::Stem stem1() const;
  [[nodiscard]] word::Stem stem2() const;
  [[nodiscard]] word::Stem stem3() const;
  [[nodiscard]] word::Stem stem4() const;
  [[nodiscard]] latin::Entry grammar() const;
  [[nodiscard]] latin::Age age() const;
  [[nodiscard]] latin::Area area() const;
  [[nodiscard]] latin::Geography geography() const;
  [[nodiscard]] latin::Frequency frequency() const;
  [[nodiscard]] latin::Source source() const;
  [[nodiscard]] std::string_view senses() const;
  [[nodiscard]] std::string_view packon() const;

private:
  DictlineRow entry;
};

class Inflects {
public:
  explicit constexpr Inflects(InflectsRow entry) : entry{entry} {}

  [[nodiscard]] latin::Inflection grammar() const;
  [[nodiscard]] latin::StemKey stemKey() const;
  [[nodiscard]] latin::CharacterCount characterCount() const;
  [[nodiscard]] std::string_view ending() const;
  [[nodiscard]] latin::Age age() const;
  [[nodiscard]] latin::Frequency frequency() const;

private:
  InflectsRow entry;
};

class Uniques {
public:
  explicit constexpr Uniques(source::UniquesIndex entry) : entry{entry} {}

  [[nodiscard]] std::string_view form() const;
  [[nodiscard]] latin::Entry grammar() const;
  [[nodiscard]] latin::Inflection inflection() const;
  [[nodiscard]] latin::Age age() const;
  [[nodiscard]] latin::Area area() const;
  [[nodiscard]] latin::Geography geography() const;
  [[nodiscard]] latin::Frequency frequency() const;
  [[nodiscard]] latin::Source source() const;
  [[nodiscard]] std::string_view senses() const;

private:
  source::UniquesIndex entry;
};

class Addons {
public:
  explicit constexpr Addons(source::AddonsIndex entry) : entry{entry} {}

  [[nodiscard]] latin::Addon addon() const;
  [[nodiscard]] std::string_view meaning() const;
  [[nodiscard]] latin::AddonKind kind() const;

private:
  source::AddonsIndex entry;
};

} // namespace word
