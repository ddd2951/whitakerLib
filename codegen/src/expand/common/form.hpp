#pragma once

#include <string_view>
#include <variant>
#include <type_traits>

#include "latin.hpp"
#include "word/word.hpp"

namespace expand {

using DictlineRow = word::DictlineRow;
using InflectsRow = word::InflectsRow;
using UniquesRow = word::UniquesRow;

struct Joined {
  DictlineRow entry{};
  InflectsRow inflection{};
  latin::StemKey column{};
};
static_assert(std::is_trivially_copyable_v<Joined>);

struct Unique {
  UniquesRow entry{};
};
static_assert(std::is_trivially_copyable_v<Unique>);

using Origin = std::variant<Joined, Unique>;
static_assert(std::is_trivially_copyable_v<Origin>);

struct Form {
  std::string_view spelling{};
  Origin origin{};
};
static_assert(std::is_trivially_copyable_v<Form>);

} // namespace expand
