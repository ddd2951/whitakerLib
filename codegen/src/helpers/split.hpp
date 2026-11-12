#pragma once

#include "error/error.hpp"
#include "helpers/fields.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace helpers {

template <std::size_t max>
Fields<max> split(const std::string_view text, const std::string_view separators, const std::string_view where) {
  static_assert(std::is_trivially_copyable_v<Fields<max>>);
  Fields<max> fields{};
  std::size_t begin = text.find_first_not_of(separators);
  while (begin != std::string_view::npos) {
    if (fields.count == max)
      error::invalid("more than " + std::to_string(max) + " fields", where);
    const std::size_t end = text.find_first_of(separators, begin);
    fields.at[fields.count++] = text.substr(begin, end - begin);
    begin = text.find_first_not_of(separators, end);
  }
  return fields;
}

} // namespace helpers
