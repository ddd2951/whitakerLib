#pragma once

#include <cstddef>
#include <meta>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace util {

template <typename E> inline constexpr auto kEnumerators = std::define_static_array(std::meta::enumerators_of(^^E));

template <typename E> [[nodiscard]] constexpr std::string_view enumToSv(E value) noexcept {
  template for (constexpr auto e : kEnumerators<E>) if ([:e:] == value) return std::meta::identifier_of(e);
  return {};
}

template <typename E> [[nodiscard]] constexpr std::optional<E> trySvToEnum(std::string_view name) noexcept {
  template for (constexpr auto e : kEnumerators<E>) if (std::meta::identifier_of(e) == name) return [:e:];
  return std::nullopt;
}

template <typename T> void appendValue(std::string& out, const T& value);

template <typename T>
inline constexpr auto kMembers =
    std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()));

template <typename T>
inline constexpr bool kIsTypedNumber = kMembers<T>.size() == 1 && std::meta::identifier_of(kMembers<T>[0]) == "value";

template <typename T> void appendMembers(std::string& out, const T& value) {
  if constexpr (kIsTypedNumber<T>) {
    appendValue(out, value.[:kMembers<T>[0]:]);
  } else {
    out += '{';
    bool first = true;
    template for (constexpr auto member : kMembers<T>) {
      if (!first)
        out += ", ";
      first = false;
      out += std::meta::identifier_of(member);
      out += '=';
      appendValue(out, value.[:member:]);
    }
    out += '}';
  }
}

template <typename T> void appendValue(std::string& out, const T& value) {
  if constexpr (std::is_enum_v<T>) {
    const std::string_view name = enumToSv(value);
    out += name.empty() ? std::to_string(+std::to_underlying(value)) : std::string{name};
  } else if constexpr (std::is_same_v<T, bool>) {
    out += value ? "true" : "false";
  } else if constexpr (std::is_arithmetic_v<T>) {
    out += std::to_string(+value);
  } else if constexpr (std::is_convertible_v<const T&, std::string_view>) {
    out += '"';
    out += std::string_view{value};
    out += '"';
  } else if constexpr (requires { std::variant_size<T>::value; }) {
    std::visit(
        [&]<typename A>(const A& alternative) {
          out += std::meta::identifier_of(^^A);
          if constexpr (!kMembers<A>.empty())
            appendMembers(out, alternative);
        },
        value);
  } else if constexpr (requires { std::tuple_size<T>::value; }) {
    out += '[';
    for (std::size_t i = 0; i < value.size(); ++i) {
      if (i != 0)
        out += ", ";
      appendValue(out, value[i]);
    }
    out += ']';
  } else {
    appendMembers(out, value);
  }
}

template <typename T> [[nodiscard]] std::string describe(const T& value) {
  std::string out;
  appendValue(out, value);
  return out;
}

template <typename T> [[nodiscard]] std::string differences(const T& before, const T& after) {
  std::string out;
  template for (constexpr auto member : kMembers<T>) {
    if (before.[:member:] != after.[:member:]) {
      out += "  ";
      out += std::meta::identifier_of(member);
      out += ": ";
      out += describe(before.[:member:]);
      out += " -> ";
      out += describe(after.[:member:]);
      out += '\n';
    }
  }
  return out;
}

} // namespace util
