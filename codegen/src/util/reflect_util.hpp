#pragma once

#include <array>
#include <cstddef>
#include <meta>
#include <optional>
#include <string_view>
#include <utility>

namespace util {

template <typename E> struct EnumPair {
  std::string_view name;
  E value;
};

template <typename E>
inline constexpr auto kEnumTable =
    []<std::size_t... Is>(std::index_sequence<Is...>) {
      return std::array<EnumPair<E>, sizeof...(Is)>{EnumPair<E>{
          std::meta::identifier_of(std::meta::enumerators_of(^^E)[Is]),
          std::meta::extract<E>(std::meta::enumerators_of(^^E)[Is])}...};
    }(std::make_index_sequence<std::meta::enumerators_of(^^E).size()>{});

template <typename E>
[[nodiscard]] constexpr std::string_view enumToSv(E value) noexcept {
  for (const EnumPair<E>& entry : kEnumTable<E>)
    if (entry.value == value)
      return entry.name;
  return {};
}

template <typename E>
[[nodiscard]] constexpr std::optional<E>
trySvToEnum(std::string_view name) noexcept {
  for (const EnumPair<E>& entry : kEnumTable<E>)
    if (entry.name == name)
      return entry.value;
  return std::nullopt;
}

} // namespace util
