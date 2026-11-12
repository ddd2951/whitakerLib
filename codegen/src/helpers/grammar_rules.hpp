#pragma once

#include "emitter/relationship_emitter/image_data.hpp"
#include "latin.hpp"

#include <cstdint>
#include <type_traits>
#include <variant>

namespace helpers {

using namespace emitter;

[[nodiscard]] inline std::uint16_t grammarRules(const latin::Entry& grammar, const std::uint16_t packon) {
  const latin::Part part = partOf(grammar);
  std::uint16_t rules = static_cast<std::uint16_t>(packon << rel::kRulePackonShift);
  if (part == latin::Part::CONJ || part == latin::Part::INTERJ)
    rules |= rel::kRuleInterjOrConj;
  std::visit(
      [&rules](const auto& value) {
        using Grammar = std::remove_cvref_t<decltype(value)>;
        if constexpr (requires { value.declension; }) {
          if (value.declension.value == 9 && value.declensionVariant.value == 8)
            rules |= rel::kRuleAbbreviation;
        }
        if constexpr (std::is_same_v<Grammar, latin::Verb>) {
          if (value.conjugation.value == 3 && value.conjugationVariant.value == 1)
            rules |= rel::kRuleConjThreeOne;
        }
      },
      grammar);
  return rules;
}

} // namespace helpers
