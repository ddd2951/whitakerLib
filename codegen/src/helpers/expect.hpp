#pragma once

#include "error/error.hpp"
#include "helpers/fields.hpp"
#include "latin.hpp"
#include "util/reflect_util.hpp"

#include <cstddef>
#include <string>
#include <string_view>

namespace helpers {

template <std::size_t max>
void expect(const Fields<max>& f, const std::size_t at, const std::size_t arity, const latin::Part part,
            const std::string_view where) {
  if (f.count < at + arity)
    error::invalid(std::string{util::enumToSv(part)} + " grammar of " + std::to_string(f.count - at) + " fields (" +
                       std::to_string(arity) + " needed)",
                   where);
}

} // namespace helpers
