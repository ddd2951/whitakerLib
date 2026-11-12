#include "execution_steps/steps.hpp"

#include "helpers/description_id.hpp"
#include "storage/description_ids.hpp"
#include "storage/image.hpp"

void execution_steps::finish_image_data::assignRuleDescriptions() {
  using helpers::descriptionId;
  using storage::descriptionIds;
  using storage::image;
  for (auto& r : image.twoEnded.rules) {
    const auto g = latin::packed::grammarOf(latin::packed::Reading{latin::packed::headChange(r.reading.rex),
                                                                   latin::packed::headPart(r.reading.rex),
                                                                   0,
                                                                   0,
                                                                   0,
                                                                   {r.reading.g[0], r.reading.g[1], r.reading.g[2]},
                                                                   0});
    r.description = descriptionId(g, image, descriptionIds);
  }
}
