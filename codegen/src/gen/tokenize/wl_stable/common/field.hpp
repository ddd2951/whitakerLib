#pragma once

#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/split.hpp"
#include "types/domain.hpp"
#include "util/reflect_util.hpp"

namespace wl_stable::tokenize::internal {

template <std::size_t max>
[[noreturn]] void refuse(const Fields<max>& f, std::size_t i,
                         std::string_view where) {
  error::fatal(std::string{where} + ": field " + std::to_string(i + 1) + " '" +
               std::string{f.at[i]} + "' is not in its domain");
}

template <typename T, std::size_t max>
T number(const Fields<max>& f, std::size_t i, std::string_view where) {
  const std::string_view token = f.at[i];
  typename T::representation_type raw{};
  const auto [end, ec] =
      std::from_chars(token.data(), token.data() + token.size(), raw);
  if (ec != std::errc{} || end != token.data() + token.size())
    refuse(f, i, where);
  const T value{raw};
  if (!domain::isValid(value))
    refuse(f, i, where);
  return value;
}

// NOTE: NONE is the not-stated value, never written.
template <typename E, std::size_t max>
E name(const Fields<max>& f, std::size_t i, std::string_view where) {
  const auto value = util::trySvToEnum<E>(f.at[i]);
  if (!value || *value == E::NONE)
    refuse(f, i, where);
  return *value;
}

template <typename E, std::size_t max>
E letter(const Fields<max>& f, std::size_t i, std::string_view where) {
  if (f.at[i].size() != 1)
    refuse(f, i, where);
  for (const auto& entry : util::kEnumTable<E>)
    if (entry.value != E::NONE && static_cast<char>(entry.value) == f.at[i][0])
      return entry.value;
  refuse(f, i, where);
}

} // namespace wl_stable::tokenize::internal
