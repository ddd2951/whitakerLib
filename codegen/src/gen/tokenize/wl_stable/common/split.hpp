#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "error/error.hpp"

namespace wl_stable::tokenize::internal {

template <std::size_t max> struct Fields {
  std::array<std::string_view, max> at{};
  std::size_t count = 0;
};

template <std::size_t max>
Fields<max> split(std::string_view text, std::string_view separators,
                  std::string_view where) {
  Fields<max> fields;
  std::size_t begin = text.find_first_not_of(separators);
  while (begin != std::string_view::npos) {
    if (fields.count == max)
      error::fatal(std::string{where} + ": more than " + std::to_string(max) +
                   " fields");
    const std::size_t end = text.find_first_of(separators, begin);
    fields.at[fields.count++] = text.substr(begin, end - begin);
    begin = text.find_first_not_of(separators, end);
  }
  return fields;
}

} // namespace wl_stable::tokenize::internal
