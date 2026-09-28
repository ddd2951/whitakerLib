#pragma once

#include <cstdint>
#include <string_view>
#include <variant>

#include "shared/semantic/value.hpp"
#include "latin.hpp"
#include "word/word.hpp"

namespace expand {

struct UniquesRowTag;

using DictlineRow = word::DictlineRow;
using InflectsRow = word::InflectsRow;
using UniquesRow = semantic::Value<UniquesRowTag, std::uint32_t>;

struct Joined {
  DictlineRow entry;
  InflectsRow inflection;
  latin::StemKey column;
};

struct Unique {
  UniquesRow entry;
};

// INFO: A unique adjective re-read as an adverb by Fix_Adverb. The only way a
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

} // namespace expand
