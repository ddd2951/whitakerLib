#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "error/error.hpp"
#include "helpers/addon_at.hpp"
#include "storage/addon_rows.hpp"
#include "storage/packons.hpp"
#include "src/search/relationship_schema.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using namespace emitter;
using helpers::addonAt;
using storage::AddonRows;

// NOTE: Ordinals start at 1 so that an entry naming no packon reads as 0.
[[nodiscard]] std::map<std::string, std::uint16_t> packonOrdinals(const AddonRows& addonRows) {
  const std::vector<std::size_t>& packons = addonRows[std::to_underlying(latin::AddonKind::packon)];
  std::map<std::string, std::uint16_t> result{};
  for (std::size_t index = 0; index < packons.size(); ++index) {
    const std::uint16_t ordinal = static_cast<std::uint16_t>(index + 1);
    if (ordinal > rel::kRulePackonMask)
      error::fatal("image: more packons than the class rules can name");
    const auto packon = std::get<latin::addon::Tackon>(addonAt(packons[index]).addon);
    result.emplace(normalize(packon.fix), ordinal);
  }
  return result;
}

} // namespace

void execution_steps::image_data::numberPackons() {
  using storage::addonRows;
  using storage::packons;
  packons = packonOrdinals(addonRows);
}
