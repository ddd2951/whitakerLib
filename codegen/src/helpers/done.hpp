#pragma once

#include "error/error.hpp"
#include "helpers/fields.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace helpers {

template <std::size_t max> void done(const Fields<max>& f, const std::size_t at, const std::string_view where) {
  if (f.count != at)
    error::invalid(std::to_string(f.count - at) + " fields after the grammar", where);
}

} // namespace helpers
