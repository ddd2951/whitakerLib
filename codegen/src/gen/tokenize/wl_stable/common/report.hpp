#pragma once

#include <cstddef>
#include <map>
#include <string>

#include "util/reflect_util.hpp"

namespace wl_stable::tokenize::internal {

// One ledger line of a histogram: " value count value count ...".
template <typename E>
std::string counts(const std::map<E, std::size_t>& histogram) {
  std::string out;
  for (const auto& [value, count] : histogram)
    out +=
        " " + std::string{util::enumToSv(value)} + " " + std::to_string(count);
  return out;
}

inline std::string counts(const std::map<std::size_t, std::size_t>& histogram) {
  std::string out;
  for (const auto& [value, count] : histogram)
    out += " " + std::to_string(value) + ":" + std::to_string(count);
  return out;
}

} // namespace wl_stable::tokenize::internal
