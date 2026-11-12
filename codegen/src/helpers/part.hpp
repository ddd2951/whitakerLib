#pragma once

#include "error/error.hpp"
#include "latin.hpp"
#include "util/reflect_util.hpp"

#include <span>
#include <string>
#include <string_view>

namespace helpers {

inline latin::Part part(const std::span<const latin::Part> parts, const std::string_view file,
                        const std::string_view token, const std::string_view where) {
  for (const latin::Part part : parts)
    if (util::enumToSv(part) == token)
      return part;
  error::invalid(std::string{file} + " part '" + std::string{token} + "'", where);
}

} // namespace helpers
