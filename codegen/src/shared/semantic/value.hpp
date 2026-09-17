#pragma once

#include <type_traits>

namespace semantic {

// NOTE: Tag is type metadata only; Value supplies no arithmetic, conversion,
//  clamping, or range policy.
template <typename Tag, typename Rep>
  requires std::is_arithmetic_v<Rep>
struct Value {
  using tag_type = Tag;
  using representation_type = Rep;

  constexpr Value() = default;

  explicit constexpr Value(Rep v) : value{v} {}

  Rep value{};

  friend constexpr bool operator==(const Value&, const Value&) = default;
};

} // namespace semantic
