#include "execution_steps/steps.hpp"

#include "expand/common/licensing.hpp"
#include "helpers/class_row.hpp"
#include "storage/class_grammars.hpp"
#include "storage/description_ids.hpp"
#include "storage/ending_order.hpp"
#include "storage/image.hpp"
#include "word/word.hpp"

#include <cstdint>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using namespace emitter;
using helpers::classRow;
using storage::DescriptionIds;
using storage::EndingOrder;

// NOTE: For prefixes: they start from a stem.
//  The runtime checks the key.
void licenseClasses(const std::vector<latin::Entry>& classGrammars, const EndingOrder& endingOrder, ImageData& image,
                    DescriptionIds& descriptionIds) {
  for (std::uint16_t id = 0; id < classGrammars.size(); ++id) {
    const latin::Entry& grammar = classGrammars[id];
    rel::ClassRecord& record = image.classes[id];
    const bool packon =
        partOf(grammar) == latin::Part::packon && ((record.rules >> rel::kRulePackonShift) & rel::kRulePackonMask) != 0;
    record.rowStart = static_cast<std::uint32_t>(image.classRows.size());
    for (std::uint32_t position = 0; position < endingOrder.size(); ++position) {
      const word::InflectsEntry& inflection = word::entry(word::InflectsRow{endingOrder[position].second});
      if (!(packon ? expand::internal::packonLicenses(grammar, inflection.grammar)
                   : expand::internal::licenses(grammar, inflection.grammar)))
        continue;
      classRow(grammar, inflection, position, image, descriptionIds);
    }
    record.rowCount = static_cast<std::uint16_t>(image.classRows.size() - record.rowStart);
  }
}

} // namespace

void execution_steps::image_data::licenseDictionaryClasses() {
  using storage::classGrammars;
  using storage::descriptionIds;
  using storage::endingOrder;
  using storage::image;
  licenseClasses(classGrammars, endingOrder, image, descriptionIds);
}
