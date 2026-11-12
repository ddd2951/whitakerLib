#include "execution_steps/steps.hpp"

#include "storage/image_spellings.hpp"
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
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using helpers::check;

struct ExpectedRow {
  std::string word{};
  expand::ResultIndex result{};
};

void validateSemanticRows(const std::vector<std::string>& spellings) {
  std::vector<ExpectedRow> expected{};
  expected.reserve(expand::resultCount());
  for (std::size_t index = 0; index < expand::resultCount(); ++index) {
    const expand::ResultIndex result{index};
    if (expand::fate(result) != expand::Fate::kept)
      continue;
    expected.push_back(ExpectedRow{emitter::normalize(expand::spelling(result)), result});
  }

  std::ranges::stable_sort(expected, {}, &ExpectedRow::word);

  std::size_t spellingOrdinal = 0;
  for (std::size_t row = 0; row < expected.size();) {
    const std::size_t begin = row;
    while (row < expected.size() && expected[row].word == expected[begin].word)
      ++row;
    const std::size_t count = row - begin;
    check(spellingOrdinal < spellings.size() && spellings[spellingOrdinal] == expected[begin].word,
          "every kept spelling, in order");

    const rel::Program program = rel::lookup(expected[begin].word);
    check(program.count == count, "every row of each spelling, in order");
    std::uint32_t cursor = program.begin;
    std::uint16_t dense = std::numeric_limits<std::uint16_t>::max();
    for (std::size_t index = begin; index < row; ++index) {
      const rel::Result actual = rel::next(cursor, dense);
      const char* const meaning = rel::dictionaryMeaning(actual.dictionary);
      check(actual.stemPattern != nullptr && meaning != nullptr,
            "every row's stem pattern and meaning are in the string pool");
      check(actual.stemLength <= expected[index].word.size(), "every printed stem fits its spelling");
      const emitter::Rendered rendered = emitter::render(expand::reading(expected[index].result));
      char stem[facts::kMaxWordCharacters];
      const std::string_view printed{stem,
                                     rel::printStem(expected[index].word, actual.stemLength, actual.stemPattern, stem)};
      check(rendered.orth == printed && rendered.meaning == meaning && rendered.analysis == actual.grammar,
            "every row's stem, meaning and grammar");
    }
    ++spellingOrdinal;
  }

  check(spellingOrdinal == spellings.size(), "no spelling expansion didn't keep");
}

} // namespace

void execution_steps::image_file::checkImageReadings() { validateSemanticRows(storage::imageSpellings); }
