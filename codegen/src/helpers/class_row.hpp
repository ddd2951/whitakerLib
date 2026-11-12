#pragma once

#include "helpers/description_id.hpp"
#include "word/word.hpp"

#include <cstdint>

namespace helpers {

inline void classRow(const latin::Entry& grammar, const word::InflectsEntry& inflection, const std::uint32_t position,
                     ImageData& image, DescriptionIds& descriptionIds) {
  const latin::Analysis analysis = printed(analysisOf(grammar, inflection.grammar, inflection.stemKey));
  const std::uint16_t description = descriptionId(analysis, image, descriptionIds);
  image.classRows.push_back(rel::ClassRowRecord{static_cast<std::uint16_t>(position), description});
}

} // namespace helpers
