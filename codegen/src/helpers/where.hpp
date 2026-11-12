#pragma once

#include "source/source.hpp"

#include <string>
#include <string_view>

namespace helpers {

template <typename Index> std::string where(const std::string_view path, const Index entry) {
  return std::string{path} + ":" + std::to_string(source::lineNumber(entry));
}

} // namespace helpers
