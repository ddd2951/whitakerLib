#pragma once

#include "helpers/fields.hpp"
#include "helpers/refuse.hpp"
#include "latin.hpp"

#include <charconv>
#include <cstddef>
#include <string_view>
#include <system_error>

namespace helpers {

template <typename T, std::size_t max>
T number(const Fields<max>& f, const std::size_t i, const std::string_view where) {
  const std::string_view token = f.at[i];
  decltype(T::value) raw{};
  const auto [end, ec] = std::from_chars(token.data(), token.data() + token.size(), raw);
  if (ec != std::errc{} || end != token.data() + token.size())
    refuse(f, i, where);
  const T value{raw};
  if (!latin::isValid(value))
    refuse(f, i, where);
  return value;
}

} // namespace helpers
