#pragma once

#include "rendered_analysis.hpp"
#include "src/search/relationship_schema.hpp"
#include "types/grammar.hpp"

#include <cstdint>
#include <type_traits>
#include <utility>
#include <variant>

namespace emitter {

[[nodiscard]] inline whitaker::relationship::Entry
entryValues(const latin::Entry& grammar, latin::Area area,
            latin::Geography geography, latin::Source source) {
  whitaker::relationship::Entry entry{.part = partOf(grammar),
                                      .area = area,
                                      .geography = geography,
                                      .source = source};
  std::visit(
      [&entry]<typename T>(const T& value) {
        if constexpr (requires { value.declension; }) {
          entry.which = value.declension.value;
          entry.variant = value.declensionVariant.value;
        } else if constexpr (requires { value.conjugation; }) {
          entry.which = value.conjugation.value;
          entry.variant = value.conjugationVariant.value;
        }
        if constexpr (requires { value.gender; })
          entry.gender = value.gender;
        if constexpr (requires { value.comparison; })
          entry.comparison = value.comparison;
        if constexpr (requires { value.numeralSort; })
          entry.numeralSort = value.numeralSort;
        if constexpr (requires { value.numeralValue; })
          entry.numeralValue = value.numeralValue;
        if constexpr (std::is_same_v<T, latin::Noun>)
          entry.kind = std::to_underlying(value.nounKind);
        else if constexpr (std::is_same_v<T, latin::Pronoun>)
          entry.kind = std::to_underlying(value.pronounKind);
        else if constexpr (std::is_same_v<T, latin::Packon>)
          entry.kind = std::to_underlying(value.packonKind);
        else if constexpr (std::is_same_v<T, latin::Verb>)
          entry.kind = std::to_underlying(value.verbKind);
      },
      grammar);
  return entry;
}

} // namespace emitter
