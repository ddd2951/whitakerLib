#pragma once

#include "latin.hpp"
#include "types/grammar.hpp"

#include <cstddef>
#include <type_traits>

namespace helpers {

template <typename T> struct Parsed {
  T value{};
  std::size_t fields{};
};
static_assert(std::is_trivially_copyable_v<Parsed<latin::Entry>>);
static_assert(std::is_trivially_copyable_v<Parsed<latin::Inflection>>);
static_assert(std::is_trivially_copyable_v<Parsed<latin::addon::Target>>);

} // namespace helpers
