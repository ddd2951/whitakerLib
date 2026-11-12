#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "storage/ending_order.hpp"
#include "storage/image.hpp"
#include "word/word.hpp"

#include <algorithm>
#include <cstdint>
#include <variant>

namespace rel = whitaker::relationship;

namespace {

using namespace emitter;
using storage::EndingOrder;

[[nodiscard]] std::uint8_t inflectionAllow(const word::InflectsEntry& inflection) {
  const auto* const verb = std::get_if<latin::inflected::Verb>(&inflection.grammar);
  if (verb == nullptr)
    return 0;
  return latin::allowOf(verb->tense, verb->voice, verb->mood, verb->person, verb->number,
                        inflection.characterCount.value == 0)
      .bits;
}

// INFO: Run_Inflections scans INFLECTS.LAT rows only, never the synthetic ones.
[[nodiscard]] EndingOrder orderInflections(ImageData& image) {
  EndingOrder endingOrder{};
  endingOrder.reserve(source::scheme::inflects::kEntriesPerFile);
  for (std::uint32_t row = 0; row < source::scheme::inflects::kEntriesPerFile; ++row)
    endingOrder.emplace_back(normalize(word::entry(word::InflectsRow{row}).ending), row);
  std::ranges::sort(endingOrder);

  image.inflections.reserve(endingOrder.size());
  for (const auto& [ending, row] : endingOrder) {
    const word::InflectsEntry& inflection = word::entry(word::InflectsRow{row});
    image.inflections.push_back(rel::InflectionRecord{.ending = image.strings.intern(ending),
                                                      .stemKey = inflection.stemKey.value,
                                                      .allow = inflectionAllow(inflection),
                                                      .age = inflection.age,
                                                      .frequency = inflection.frequency});
  }
  for (std::uint32_t row = 0; row < endingOrder.size();) {
    const std::uint32_t begin = row;
    while (row < endingOrder.size() && endingOrder[row].first == endingOrder[begin].first)
      ++row;
    image.endings.push_back(rel::EndingRecord{image.inflections[begin].ending, static_cast<std::uint16_t>(begin),
                                              static_cast<std::uint16_t>(row - begin)});
  }
  return endingOrder;
}

} // namespace

void execution_steps::image_data::orderInflections() {
  using ::orderInflections;
  using storage::endingOrder;
  using storage::image;
  endingOrder = orderInflections(image);
}
