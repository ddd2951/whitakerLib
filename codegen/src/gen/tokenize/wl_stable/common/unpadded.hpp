#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <string_view>

namespace wl_stable::tokenize::internal {

template <std::size_t width>
std::string_view unpadded(std::span<const char, width> slice, char padding) {
  const auto end =
      std::find_if(slice.rbegin(), slice.rend(), [padding](char c) {
        return c != padding;
      }).base();
  return {slice.data(), static_cast<std::size_t>(end - slice.begin())};
}

} // namespace wl_stable::tokenize::internal
