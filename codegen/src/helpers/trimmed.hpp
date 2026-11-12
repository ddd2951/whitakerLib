#pragma once

#include <cstddef>
#include <string_view>

namespace helpers {

inline std::string_view trimmed(const std::string_view text, const char padding) {
  const std::size_t end = text.find_last_not_of(padding);
  return end == std::string_view::npos ? std::string_view{} : text.substr(0, end + 1);
}

} // namespace helpers
