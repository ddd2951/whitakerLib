#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include "latin.hpp"

// INFO: Inflections_Package."<" on Quality_Record.
namespace expand::quality {

// INFO: Part_Of_Speech_Type's order.
[[nodiscard]] constexpr unsigned pofsPos(latin::Part part) noexcept {
  switch (part) {
  case latin::Part::X:
    return 0;
  case latin::Part::N:
    return 1;
  case latin::Part::PRON:
    return 2;
  case latin::Part::PACK:
    return 3;
  case latin::Part::ADJ:
    return 4;
  case latin::Part::NUM:
    return 5;
  case latin::Part::ADV:
    return 6;
  case latin::Part::V:
    return 7;
  case latin::Part::VPAR:
    return 8;
  case latin::Part::SUPINE:
    return 9;
  case latin::Part::PREP:
    return 10;
  case latin::Part::CONJ:
    return 11;
  case latin::Part::INTERJ:
    return 12;
  case latin::Part::NONE:
    return 13;
  }
  return 13;
}

template <typename T> [[nodiscard]] constexpr unsigned u(T value) noexcept {
  if constexpr (std::is_enum_v<T>)
    return std::to_underlying(value);
  else
    return value.value;
}

[[nodiscard]] inline auto sortFields(const latin::Analysis& q) {
  using enum latin::Part;
  const unsigned none = 0;
  switch (q.part) {
  case N:
  case PRON:
  case PACK:
  case VPAR:
  case SUPINE:
    return std::tuple{unsigned{q.which}, u(q.variant), u(q.number), u(q.caseOf), u(q.gender), none, none};
  case ADJ:
    return std::tuple{unsigned{q.which}, u(q.variant), u(q.number), u(q.caseOf), u(q.gender), u(q.comparison), none};
  case NUM:
    return std::tuple{unsigned{q.which}, u(q.variant), u(q.number), u(q.caseOf), u(q.gender), u(q.numeralSort), none};
  case V:
    return std::tuple{unsigned{q.which}, u(q.variant), u(q.number), u(q.tense), u(q.voice), u(q.mood), u(q.person)};
  case ADV:
    return std::tuple{u(q.comparison), none, none, none, none, none, none};
  case PREP:
    return std::tuple{u(q.caseOf), none, none, none, none, none, none};
  default:
    return std::tuple{none, none, none, none, none, none, none};
  }
}

[[nodiscard]] inline bool inflectionsLess(const latin::Analysis& left, const latin::Analysis& right) {
  if (left.part != right.part)
    return pofsPos(left.part) < pofsPos(right.part);
  return sortFields(left) < sortFields(right);
}

} // namespace expand::quality
