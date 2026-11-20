#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "storage/image.hpp"

#include <cstdint>
#include <limits>

void execution_steps::finish_image_data::groupDictionaryStems() {
  using storage::image;
  if (image.stems.size() > std::numeric_limits<std::uint16_t>::max())
    error::fatal("image: stem rows do not fit a u16");
  auto& runs = image.dictionaryStemRuns;
  runs.assign(image.dictionaries.size() + 1, 0);
  for (const auto& stem : image.stems) {
    if (stem.dictionary >= image.dictionaries.size())
      error::fatal("image: a stem row names no dictionary word");
    ++runs[std::size_t{stem.dictionary} + 1];
  }
  for (std::size_t i = 1; i < runs.size(); ++i)
    runs[i] = static_cast<std::uint16_t>(runs[i] + runs[i - 1]);
  auto next = runs;
  image.dictionaryStems.assign(image.stems.size(), 0);
  for (std::size_t row = 0; row < image.stems.size(); ++row)
    image.dictionaryStems[next[image.stems[row].dictionary]++] = static_cast<std::uint16_t>(row);
}
