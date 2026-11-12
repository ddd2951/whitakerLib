#pragma once

#include "error/error.hpp"

#include <algorithm>
#include <string>
#include <string_view>

namespace helpers {

inline std::string_view printable(const std::string_view text, const std::string_view what,
                                  const std::string_view where) {
  if (text.empty())
    error::invalid("empty " + std::string{what}, where);
  const auto low = std::ranges::find_if(text, [](const char c) { return c < ' ' || c > '~'; });
  if (low != text.end())
    error::invalid(std::string{what} + " with a byte outside printable ASCII at offset " +
                       std::to_string(low - text.begin()),
                   where);
  return text;
}

} // namespace helpers
