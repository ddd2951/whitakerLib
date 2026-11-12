#pragma once

#include "error/error.hpp"

#include <string>
#include <string_view>

namespace helpers {

inline void letters(const std::string_view text, const std::string_view what, const std::string_view where) {
  for (const char c : text)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
      error::invalid(std::string{what} + " '" + std::string{text} + "', not letters", where);
}

} // namespace helpers
