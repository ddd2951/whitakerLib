#pragma once

#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>

#include "error/error.hpp"
#include "word/split.hpp"
#include "latin.hpp"
#include "util/reflect_util.hpp"

namespace word::internal {

template <std::size_t max>
[[noreturn]] void refuse(const Fields<max>& f, std::size_t i,
                         std::string_view where) {
  error::fatal(std::string{where} + ": field " + std::to_string(i + 1) + " '" +
               std::string{f.at[i]} + "' is not in its domain");
}

template <typename T, std::size_t max>
T number(const Fields<max>& f, std::size_t i, std::string_view where) {
  const std::string_view token = f.at[i];
  decltype(T::value) raw{};
  const auto [end, ec] =
      std::from_chars(token.data(), token.data() + token.size(), raw);
  if (ec != std::errc{} || end != token.data() + token.size())
    refuse(f, i, where);
  const T value{raw};
  if (!latin::isValid(value))
    refuse(f, i, where);
  return value;
}

template <typename E, std::size_t max>
E name(const Fields<max>& f, std::size_t i, std::string_view where) {
  const auto value = util::trySvToEnum<E>(f.at[i]);
  if (!value)
    refuse(f, i, where);
  return *value;
}

} // namespace word::internal
