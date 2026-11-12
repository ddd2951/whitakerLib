#pragma once

#include "emitter/relationship_emitter/image_data.hpp"
#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "error/error.hpp"
#include "storage/description_ids.hpp"
#include "util/reflect_util.hpp"

#include <cstdint>
#include <format>
#include <limits>

namespace helpers {

using namespace emitter;
using storage::DescriptionIds;

[[nodiscard]] inline std::uint16_t descriptionId(const latin::Analysis& analysis, ImageData& image,
                                                 DescriptionIds& descriptionIds) {
  const auto [description, inserted] =
      descriptionIds.emplace(describe(analysis), static_cast<std::uint32_t>(image.grammars.size()));
  if (inserted) {
    const rel::UnpackedGrammar back = rel::unpackGrammar(rel::packGrammar(analysis));
    if (!back.valid || back.analysis != analysis)
      error::fatal(std::format("image: description \"{}\" has a field wider than its grammar bits\n{}",
                               description->first, util::describe(analysis)));
    image.grammars.push_back(analysis);
  } else if (image.grammars[description->second] != analysis) {
    error::fatal(std::format("image: description \"{}\" renders two grammars\n{}", description->first,
                             util::differences(image.grammars[description->second], analysis)));
  }
  if (description->second > std::numeric_limits<std::uint16_t>::max())
    error::fatal("image: more descriptions than 16 bits can name");
  return static_cast<std::uint16_t>(description->second);
}

} // namespace helpers
