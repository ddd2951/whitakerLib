#pragma once

#include "error/error.hpp"
#include "helpers/fields.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace helpers {

template <std::size_t max>
void labelsFollow(const Fields<max>& f, const std::size_t at, const std::size_t labels, const std::string_view after,
                  const std::string_view where) {
  if (f.count != at + labels)
    error::invalid(std::to_string(f.count - at) + " fields after the " + std::string{after} + " (" +
                       std::to_string(labels) + " expected)",
                   where);
}

} // namespace helpers
