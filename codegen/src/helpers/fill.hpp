#pragma once

#include "error/error.hpp"
#include "helpers/fields.hpp"
#include "helpers/name.hpp"
#include "helpers/number.hpp"
#include "latin.hpp"
#include "types/grammar.hpp"
#include "util/reflect_util.hpp"

#include <cstddef>
#include <meta>
#include <string>
#include <string_view>
#include <type_traits>

namespace helpers {

template <typename T, std::size_t max>
T fill(const Fields<max>& f, const std::size_t at, const std::string_view where) {
  constexpr std::size_t arity = util::kMembers<T>.size();
  if (f.count < at + arity)
    error::invalid(std::string{util::enumToSv(latin::partTag<T>())} + " grammar of " + std::to_string(f.count - at) +
                       " fields (" + std::to_string(arity) + " needed)",
                   where);
  T out{};
  std::size_t i = at;
  template for (constexpr auto member : util::kMembers<T>) {
    using M = [:std::meta::type_of(member):];
    if constexpr (std::is_enum_v<M>)
      out.[:member:] = name<M>(f, i++, where);
    else
      out.[:member:] = number<M>(f, i++, where);
  }
  return out;
}

} // namespace helpers
