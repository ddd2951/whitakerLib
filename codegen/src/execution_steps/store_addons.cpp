#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "helpers/addon_at.hpp"
#include "helpers/addon_value.hpp"
#include "storage/addon_rows.hpp"
#include "storage/image.hpp"
#include "storage/suffix_base.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using namespace emitter;
using helpers::addonAt;
using helpers::addonValue;
using storage::AddonRows;

[[nodiscard]] std::uint8_t firstLetter(const std::string_view fix) {
  return static_cast<std::uint8_t>(fix.empty() ? '\0' : fix.front());
}

[[nodiscard]] rel::AddonRecord addonRecord(const latin::addon::Prefix& prefix, const std::string_view meaning,
                                           const latin::AddonKind kind, StringSection& strings) {
  return rel::AddonRecord{
      .fix = strings.intern(normalize(prefix.fix)),
      .meaning = strings.intern(meaning),
      .kind = kind,
      .root = prefix.from,
      .targetPart = prefix.to,
      .connectingLetter = static_cast<std::uint8_t>(prefix.connect),
      .firstLetterAsSpelled = firstLetter(prefix.fix),
  };
}

void writeAddonTarget(rel::AddonRecord& result, const latin::addon::Target& value) {
  result.targetPart = latin::partOf(value);
  std::visit(
      [&result](const auto& target) {
        using Target = std::remove_cvref_t<decltype(target)>;
        if constexpr (std::is_same_v<Target, latin::Noun>) {
          result.target = {addonValue(target.declension), addonValue(target.declensionVariant),
                           addonValue(target.gender), addonValue(target.nounKind)};
        } else if constexpr (std::is_same_v<Target, latin::Pronoun>) {
          result.target = {addonValue(target.declension), addonValue(target.declensionVariant),
                           addonValue(target.pronounKind), 0};
        } else if constexpr (std::is_same_v<Target, latin::Packon>) {
          result.target = {addonValue(target.declension), addonValue(target.declensionVariant),
                           addonValue(target.packonKind), 0};
        } else if constexpr (std::is_same_v<Target, latin::Adjective>) {
          result.target = {addonValue(target.declension), addonValue(target.declensionVariant),
                           addonValue(target.comparison), 0};
        } else if constexpr (std::is_same_v<Target, latin::Numeral>) {
          result.target = {addonValue(target.declension), addonValue(target.declensionVariant),
                           addonValue(target.numeralSort), addonValue(target.numeralValue)};
        } else if constexpr (std::is_same_v<Target, latin::Adverb>) {
          result.target = {addonValue(target.comparison), 0, 0, 0};
        } else if constexpr (std::is_same_v<Target, latin::Verb>) {
          result.target = {addonValue(target.conjugation), addonValue(target.conjugationVariant),
                           addonValue(target.verbKind), 0};
        } else if constexpr (!std::is_same_v<Target, latin::addon::Any>) {
          static_assert(false, "an add-on target kind with no layout");
        }
      },
      value);
}

[[nodiscard]] rel::AddonRecord addonRecord(const latin::addon::Suffix& addon, const std::string_view meaning,
                                           const latin::AddonKind kind, StringSection& strings) {
  rel::AddonRecord result{
      .fix = strings.intern(normalize(addon.fix)),
      .meaning = strings.intern(meaning),
      .kind = kind,
      .root = addon.from,
      .rootStemKey = addon.fromKey.value,
      .targetStemKey = addon.toKey.value,
      .connectingLetter = static_cast<std::uint8_t>(addon.connect),
      .firstLetterAsSpelled = firstLetter(addon.fix),
  };
  writeAddonTarget(result, addon.grammar);
  return result;
}

[[nodiscard]] rel::AddonRecord addonRecord(const latin::addon::Tackon& tackon, const std::string_view meaning,
                                           const latin::AddonKind kind, StringSection& strings) {
  rel::AddonRecord result{
      .fix = strings.intern(normalize(tackon.fix)),
      .meaning = strings.intern(meaning),
      .kind = kind,
      .root = latin::Part::any,
      .firstLetterAsSpelled = firstLetter(tackon.fix),
  };
  writeAddonTarget(result, tackon.grammar);
  return result;
}

std::size_t internAddons(const AddonRows& addonRows, ImageData& image) {
  for (const std::vector<std::size_t>& rows : addonRows)
    for (const std::size_t row : rows) {
      const word::AddonsEntry& addon = addonAt(row);
      image.addons.push_back(
          std::visit([&](const auto& value) { return addonRecord(value, addon.meaning, addon.kind, image.strings); },
                     addon.addon));
    }
  std::size_t first = 0;
  for (std::size_t kind = 0; kind < std::to_underlying(latin::AddonKind::suffix); ++kind)
    first += addonRows[kind].size();
  return first;
}

} // namespace

void execution_steps::image_data::storeAddons() {
  using storage::addonRows;
  using storage::image;
  using storage::suffixBase;
  suffixBase = internAddons(addonRows, image);
}
