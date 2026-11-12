#include "execution_steps/steps.hpp"

#include "expand/common/licensing.hpp"
#include "helpers/addon_at.hpp"
#include "helpers/class_row.hpp"
#include "helpers/grammar_rules.hpp"
#include "storage/addon_rows.hpp"
#include "storage/description_ids.hpp"
#include "storage/ending_order.hpp"
#include "storage/image.hpp"
#include "storage/suffix_base.hpp"
#include "word/word.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using namespace emitter;
using helpers::addonAt;
using helpers::classRow;
using helpers::grammarRules;
using storage::AddonRows;
using storage::DescriptionIds;
using storage::EndingOrder;

[[nodiscard]] std::optional<latin::Entry> addonGrammar(const latin::addon::Target& value) {
  return std::visit(
      [](const auto& target) -> std::optional<latin::Entry> {
        using Target = std::remove_cvref_t<decltype(target)>;
        if constexpr (std::is_same_v<Target, latin::addon::Any> || std::is_same_v<Target, latin::Packon>)
          return std::nullopt;
        else
          return latin::Entry{target};
      },
      value);
}

void licenseSuffixes(const AddonRows& addonRows, const std::size_t suffixBase, const EndingOrder& endingOrder,
                     ImageData& image, DescriptionIds& descriptionIds) {
  const std::vector<std::size_t>& suffixes = addonRows[std::to_underlying(latin::AddonKind::suffix)];
  for (std::size_t index = 0; index < suffixes.size(); ++index) {
    const auto suffix = std::get<latin::addon::Suffix>(addonAt(suffixes[index]).addon);
    const std::optional<latin::Entry> target = addonGrammar(suffix.grammar);
    rel::AddonRecord& record = image.addons[suffixBase + index];
    if (!target)
      continue;
    const latin::Part targetPart = partOf(*target);
    record.targetRules = static_cast<std::uint8_t>(grammarRules(*target, 0));
    record.rowStart = static_cast<std::uint32_t>(image.classRows.size());
    for (std::uint32_t position = 0; position < endingOrder.size(); ++position) {
      const word::InflectsEntry& inflection = word::entry(word::InflectsRow{endingOrder[position].second});
      if (!expand::internal::licenses(*target, inflection.grammar))
        continue;
      if (!latin::stemKeyAdmits(record.targetStemKey, inflection.stemKey.value, targetPart))
        continue;
      classRow(*target, inflection, position, image, descriptionIds);
    }
    record.rowCount = static_cast<std::uint16_t>(image.classRows.size() - record.rowStart);
  }
}

} // namespace

void execution_steps::image_data::licenseSuffixes() {
  using ::licenseSuffixes;
  using storage::addonRows;
  using storage::descriptionIds;
  using storage::endingOrder;
  using storage::image;
  using storage::suffixBase;
  licenseSuffixes(addonRows, suffixBase, endingOrder, image, descriptionIds);
}
