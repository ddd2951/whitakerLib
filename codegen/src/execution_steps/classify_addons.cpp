#include "execution_steps/steps.hpp"

#include "helpers/addon_at.hpp"
#include "storage/addon_rows.hpp"

#include <cstddef>
#include <utility>

namespace {

using helpers::addonAt;
using storage::AddonRows;

[[nodiscard]] AddonRows classifyAddons() {
  AddonRows result{};
  for (std::size_t row = 0; row < source::scheme::addons::kEntriesPerFile; ++row)
    result[std::to_underlying(addonAt(row).kind)].push_back(row);
  return result;
}

} // namespace

void execution_steps::image_data::classifyAddons() {
  using ::classifyAddons;
  using storage::addonRows;
  addonRows = classifyAddons();
}
