#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "shared/semantic/value.hpp"
#include "types/domain.hpp"

namespace wl_stable::expand {

struct DictlineRowTag;
struct InflectsRowTag;
struct UniquesRowTag;

using DictlineRow = semantic::Value<DictlineRowTag, std::uint32_t>;
using InflectsRow = semantic::Value<InflectsRowTag, std::uint32_t>;
using UniquesRow = semantic::Value<UniquesRowTag, std::uint32_t>;

struct Joined {
  DictlineRow entry;
  InflectsRow inflection;
  domain::StemIndex column;
};

struct Unique {
  UniquesRow entry;
};

// INFO:  A unique adjective re-read as an adverb by Fix_Adverb. The only way a
//  unique meets an inflection row, and that row is always a synthetic one.
struct UniqueAdverb {
  UniquesRow entry;
  InflectsRow inflection;
};

using Origin = std::variant<Joined, Unique, UniqueAdverb>;

struct Form {
  std::string_view spelling;
  Origin origin;
};

} // namespace wl_stable::expand
