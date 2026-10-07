#pragma once

#include <charconv>
#include <cstddef>
#include <meta>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "error/error.hpp"
#include "word/split.hpp"
#include "latin.hpp"
#include "types/grammar.hpp"
#include "util/reflect_util.hpp"

namespace word::internal {

template <std::size_t max> [[noreturn]] void refuse(const Fields<max>& f, std::size_t i, std::string_view where) {
  error::invalid("field " + std::to_string(i + 1) + " '" + std::string{f.at[i]} + "'", where);
}

template <typename T, std::size_t max> T number(const Fields<max>& f, std::size_t i, std::string_view where) {
  const std::string_view token = f.at[i];
  decltype(T::value) raw{};
  const auto [end, ec] = std::from_chars(token.data(), token.data() + token.size(), raw);
  if (ec != std::errc{} || end != token.data() + token.size())
    refuse(f, i, where);
  const T value{raw};
  if (!latin::isValid(value))
    refuse(f, i, where);
  return value;
}

template <typename E, std::size_t max> E name(const Fields<max>& f, std::size_t i, std::string_view where) {
  const auto value = util::trySvToEnum<E>(f.at[i]);
  if (!value)
    refuse(f, i, where);
  return *value;
}

template <typename T> consteval std::optional<T> tag(std::meta::info member) {
  const auto found = std::meta::annotations_of_with_type(member, ^^T);
  if (found.empty())
    return std::nullopt;
  return std::meta::extract<T>(found[0]);
}

template <typename T, std::size_t max> T fill(const Fields<max>& f, std::size_t at, std::string_view where) {
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

template <typename T> struct Parsed {
  T value;
  std::size_t fields;
};

template <typename V, std::size_t max>
Parsed<V> byPart(latin::Part part, const Fields<max>& f, std::size_t at, std::string_view where) {
  template for (constexpr auto alternative : std::define_static_array(std::meta::template_arguments_of(^^V))) {
    using A = [:alternative:];
    constexpr std::size_t arity = util::kMembers<A>.size();
    if (latin::partTag<A>() == part)
      return {fill<A>(f, at, where), arity};
  }
  error::invalid("part " + std::string{util::enumToSv(part)}, where);
}

} // namespace word::internal
