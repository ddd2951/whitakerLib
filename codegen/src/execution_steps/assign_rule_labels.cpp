#include "execution_steps/steps.hpp"

#include "storage/image.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string_view>
#include <utility>

void execution_steps::finish_image_data::assignRuleLabels() {
  using storage::image;
  {
    // INFO: The label inflectionLabels finds for (description, the rule's spelled suffix), from the records written
    //  here; a pair without one gets the default it returns.
    std::map<std::pair<std::uint16_t, std::string_view>, std::pair<latin::Age, latin::Frequency>> labels{};
    for (const auto& l : image.inflectionLabels)
      labels.emplace(std::pair{l.description, std::string_view{image.strings.blob().c_str() + l.ending}},
                     std::pair{l.age, l.frequency});
    for (std::size_t i = 0; i < image.twoEnded.rules.size(); ++i) {
      auto& r = image.twoEnded.rules[i];
      const auto& suffix = image.twoEnded.ruleSuffixes[i];
      const auto it = labels.find({r.description, suffix});
      const auto [age, frequency] = it == labels.end() ? std::pair<latin::Age, latin::Frequency>{} : it->second;
      assert(std::to_underlying(age) <= 15 && std::to_underlying(frequency) <= 15 && suffix.size() <= 255);
      r.label = static_cast<std::uint8_t>(std::to_underlying(age) | (std::to_underlying(frequency) << 4));
      r.suffixLength = static_cast<std::uint8_t>(suffix.size());
    }
  }
}
