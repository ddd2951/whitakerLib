#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "expand/expand.hpp"
#include "storage/image.hpp"
#include "storage/source.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using namespace emitter;
using storage::SourceRow;

// NOTE: Rows keep order from expansion.
void groupSpellings(std::vector<SourceRow> source, ImageData& image) {
  // INFO: List_Sweep's Trimmed: a reading left out for its stem, age or frequency; not a duplicate.
  std::unordered_set<std::string> trimmed{};
  for (std::size_t index = 0; index < expand::resultCount(); ++index)
    if (const expand::Fate fate = expand::fate(expand::ResultIndex{index});
        fate != expand::Fate::kept && fate != expand::Fate::duplicate)
      trimmed.insert(normalize(expand::spelling(expand::ResultIndex{index})));
  std::ranges::stable_sort(source, {}, &SourceRow::word);
  image.words.reserve(source.size());
  image.analyses.reserve(source.size());
  for (std::size_t i = 0; i < source.size();) {
    const std::size_t begin = i;
    while (i < source.size() && source[i].word == source[begin].word)
      ++i;
    const std::size_t count = i - begin;
    assert(count <= rel::kMaximumResultsPerSpelling);
    image.words.push_back(Word{source[begin].word, static_cast<std::uint32_t>(image.analyses.size()),
                               static_cast<std::uint16_t>(count), trimmed.contains(source[begin].word)});
    trimmed.erase(source[begin].word);
    for (std::size_t row = begin; row < i; ++row)
      image.analyses.push_back(source[row].analysis);
  }
  // INFO: A spelling whose every reading List_Sweep trims still answers with its "*" (seque: sequor's active
  //  imperative; relin under relinque): a word with no readings.
  for (const std::string& word : trimmed)
    image.words.push_back(Word{word, static_cast<std::uint32_t>(image.analyses.size()), 0, true});
  std::ranges::sort(image.words, {}, &Word::text);
}

} // namespace

void execution_steps::image_data::groupSpellings() {
  using ::groupSpellings;
  using storage::image;
  using storage::source;
  groupSpellings(std::move(source), image);
}
