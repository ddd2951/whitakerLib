#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/entry_values.hpp"
#include "storage/image.hpp"
#include "word/word.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace {

using namespace emitter;

static_assert(word::kDictlineWords + source::scheme::uniques::kEntriesPerFile <=
              std::size_t{std::numeric_limits<std::uint16_t>::max()} + 1);

void internEntries(ImageData& image) {
  image.dictionaries.resize(word::kDictlineWords + source::scheme::uniques::kEntriesPerFile);
  image.entries.reserve(image.dictionaries.size());
  for (std::uint32_t index = 0; index < word::kDictlineWords; ++index)
    image.entries.push_back(entryValues(word::entry(word::DictlineRow{index})));
  for (std::uint32_t index = 0; index < source::scheme::uniques::kEntriesPerFile; ++index)
    image.entries.push_back(entryValues(word::entry(word::UniquesRow{index})));
}

} // namespace

void execution_steps::image_data::storeDictionaryWords() {
  using storage::image;
  internEntries(image);
}
