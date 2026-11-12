#pragma once

#include "helpers/fields.hpp"
#include "helpers/refuse.hpp"
#include "util/reflect_util.hpp"

#include <cstddef>
#include <string_view>

namespace helpers {

template <typename E, std::size_t max> E name(const Fields<max>& f, const std::size_t i, const std::string_view where) {
  const auto value = util::trySvToEnum<E>(f.at[i]);
  if (!value)
    refuse(f, i, where);
  return *value;
}

} // namespace helpers
