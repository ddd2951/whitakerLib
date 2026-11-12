#pragma once

#include "error/error.hpp"
#include "util/reflect_util.hpp"

#include <string>
#include <string_view>

namespace helpers {

template <typename E> E letter(const char c, const std::string_view name, const std::string_view where) {
  if (const auto value = util::trySvToEnum<E>(std::string_view{&c, 1}))
    return *value;
  error::invalid(std::string{name} + " '" + c + "'", where);
}

template <typename E>
E letter(const std::string_view field, const std::string_view name, const std::string_view where) {
  if (field.size() != 1)
    error::invalid(std::string{name} + " '" + std::string{field} + "'", where);
  return letter<E>(field.front(), name, where);
}

} // namespace helpers
