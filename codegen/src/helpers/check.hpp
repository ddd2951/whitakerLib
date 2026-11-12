#pragma once

#include "error/error.hpp"

#include <format>
#include <source_location>
#include <string_view>

namespace helpers {

inline void check(const bool condition, const std::string_view expectation,
                  const std::source_location location = std::source_location::current()) {
  if (!condition)
    error::fatal(std::format("image: check failed: {}", expectation), location);
}

} // namespace helpers
