#pragma once

#include "error/error.hpp"
#include "helpers/fields.hpp"
#include "helpers/fill.hpp"
#include "helpers/parsed.hpp"
#include "latin.hpp"
#include "types/grammar.hpp"
#include "util/reflect_util.hpp"

#include <cstddef>
#include <meta>
#include <string>
#include <string_view>

namespace helpers {

template <typename V, std::size_t max>
Parsed<V> byPart(const latin::Part part, const Fields<max>& f, const std::size_t at, const std::string_view where) {
  template for (constexpr auto alternative : std::define_static_array(std::meta::template_arguments_of(^^V))) {
    using A = [:alternative:];
    constexpr std::size_t arity = util::kMembers<A>.size();
    if (latin::partTag<A>() == part)
      return {fill<A>(f, at, where), arity};
  }
  error::invalid("part " + std::string{util::enumToSv(part)}, where);
}

} // namespace helpers
