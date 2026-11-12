#pragma once

#include "error/error.hpp"
#include "helpers/fields.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace helpers {

template <std::size_t max>
[[noreturn]] void refuse(const Fields<max>& f, const std::size_t i, const std::string_view where) {
  error::invalid("field " + std::to_string(i + 1) + " '" + std::string{f.at[i]} + "'", where);
}

} // namespace helpers
