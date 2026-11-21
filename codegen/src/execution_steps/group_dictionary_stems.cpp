#include "execution_steps/steps.hpp"

#include "storage/image.hpp"

#include <cassert>
#include <cstdint>
#include <limits>

void execution_steps::finish_image_data::groupDictionaryStems() {
  using storage::image;
  assert(image.stems.size() <= std::numeric_limits<std::uint16_t>::max());
  auto& runs = image.dictionaryStemRuns;
  runs.assign(image.dictionaries.size() + 1, 0);
  for (const auto& stem : image.stems) {
    assert(stem.dictionary < image.dictionaries.size());
    ++runs[std::size_t{stem.dictionary} + 1];
  }
  for (std::size_t i = 1; i < runs.size(); ++i)
    runs[i] = static_cast<std::uint16_t>(runs[i] + runs[i - 1]);
  auto next = runs;
  image.dictionaryStems.assign(image.stems.size(), 0);
  for (std::size_t row = 0; row < image.stems.size(); ++row)
    image.dictionaryStems[next[image.stems[row].dictionary]++] = static_cast<std::uint16_t>(row);
}
