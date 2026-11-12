#pragma once

#include "expand/common/reading.hpp"
#include "expand/common/stem_column.hpp"
#include "word/word.hpp"

#include <optional>
#include <variant>

namespace helpers {

using namespace expand;

// INFO: Fix_Adverb on one reading: the adverb an adjective's VOC S M makes, positive of declension 1/1 or superlative.
[[nodiscard]] inline std::optional<expand::InflectsRow> adverbOf(const Reading& reading) {
  const auto* const inflected = std::get_if<latin::inflected::Adjective>(&reading.inflection);
  if (inflected == nullptr || inflected->caseOf != latin::Case::VOC || inflected->number != latin::Number::S ||
      inflected->gender != latin::Gender::M)
    return std::nullopt;
  const auto& declared = std::get<latin::Adjective>(reading.entry);
  const latin::Comparison degree = internal::adjectiveDegree(declared, reading.key);
  if (degree == latin::Comparison::SUPER)
    return word::kAdverbSuperlative;
  if (degree == latin::Comparison::POS && declared.declension.value == 1 && declared.declensionVariant.value == 1)
    return word::kAdverbPositive;
  return std::nullopt;
}

} // namespace helpers
