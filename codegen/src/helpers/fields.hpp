#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace helpers {

template <std::size_t max> struct Fields {
  std::array<std::string_view, max> at{};
  std::size_t count = 0;
};

} // namespace helpers
