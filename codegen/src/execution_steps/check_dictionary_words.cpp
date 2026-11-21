#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/entry_values.hpp"
#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "error/error.hpp"
#include "expand/expand.hpp"
#include "facts.hpp"
#include "helpers/check.hpp"
#include "src/search/image.hpp"
#include "src/search/relationship_image.hpp"
#include "util/reflect_util.hpp"
#include "word/word.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using helpers::check;

template <typename Parsed> void validateEntry(const std::uint32_t index, const Parsed& parsed) {
  const rel::Entry expected = emitter::entryValues(parsed);
  const rel::Entry stored = rel::entry(static_cast<std::uint16_t>(index));
  if (stored != expected)
    error::fatal(
        std::format("image: entry {} does not match its parsed word\n{}", index, util::differences(expected, stored)));
  const auto dictionary =
      rel::readRecord<rel::DictionaryRecord>(rel::recordAt(*rel::sImage, rel::Section::Dictionaries, index));
  check(dictionary.age == parsed.labels.age && dictionary.frequency == parsed.labels.frequency,
        "every entry's age and frequency");
}

void validateEntries() {
  const std::uint32_t uniques = source::scheme::uniques::kEntriesPerFile;
  const std::uint32_t count = word::kDictlineWords + uniques;
  check(rel::section(rel::Section::Entries).count == count, "an entry for every parsed word");
  for (std::uint32_t index = 0; index < word::kDictlineWords; ++index)
    validateEntry(index, word::entry(word::DictlineRow{index}));
  for (std::uint32_t index = 0; index < uniques; ++index)
    validateEntry(word::kDictlineWords + index, word::entry(word::UniquesRow{index}));
}

} // namespace

void execution_steps::image_file::checkDictionaryWords() { validateEntries(); }
