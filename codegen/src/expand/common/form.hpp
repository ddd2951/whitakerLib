#pragma once

#include <string_view>
#include <variant>

#include "latin.hpp"
#include "word/word.hpp"

namespace expand {

using DictlineRow = word::DictlineRow;
using InflectsRow = word::InflectsRow;
using UniquesRow = word::UniquesRow;

struct Joined {
  DictlineRow entry;
  InflectsRow inflection;
  latin::StemKey column;
};

struct Unique {
  UniquesRow entry;
};

using Origin = std::variant<Joined, Unique>;

struct Form {
  std::string_view spelling;
  Origin origin;
};

} // namespace expand
