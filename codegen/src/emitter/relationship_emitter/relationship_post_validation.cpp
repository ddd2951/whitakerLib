#include "relationship_post_validation.hpp"

#include "entry_values.hpp"
#include "error/error.hpp"
#include "expand/expand.hpp"
#include "facts.hpp"
#include "rendered_analysis.hpp"
#include "src/search/image.hpp"
#include "src/search/relationship_image.hpp"
#include "util/file_load.hpp"
#include "util/reflect_util.hpp"
#include "word/word.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <print>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

void check(bool condition, std::string_view expectation,
           std::source_location location = std::source_location::current()) {
  if (!condition)
    error::fatal(std::format("image: check failed: {}", expectation), location);
}

struct ExpectedRow {
  std::string word;
  expand::ResultIndex result;
};

void validateSemanticRows(const std::vector<std::string>& spellings) {
  std::vector<ExpectedRow> expected;
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

template <typename Parsed> void validateEntry(std::uint32_t index, const Parsed& parsed) {
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
  std::println("  entries: {} parsed words round-trip exactly", count);
}

[[nodiscard]] std::uint64_t count(rel::Section section) { return rel::section(section).count; }

template <typename Record> [[nodiscard]] bool every(rel::Section section, const auto& fits) {
  for (std::uint64_t i = 0; i < count(section); ++i)
    if (!fits(rel::readRecord<Record>(rel::recordAt(*rel::sImage, section, i))))
      return false;
  return true;
}

// The records only the fallback reads, which the walk above never reaches: each stays inside its tables, and the two
// the fallback searches are sorted.
void validateFallbackRecords() {
  const std::uint64_t pool = rel::section(rel::Section::Strings).bytes;
  const auto inPool = [pool](std::uint32_t offset) { return offset < pool; };
  const auto text = [](std::uint32_t offset) { return std::string_view{rel::sImage->strings + offset}; };
  const auto part = [](latin::Part value) { return latin::isPart(std::to_underlying(value)); };

  check(pool != 0 && rel::sImage->strings[pool - 1] == '\0', "the string pool ends with a terminator");
  check(every<rel::DictionaryRecord>(rel::Section::Dictionaries,
                                     [&](const rel::DictionaryRecord& r) { return inPool(r.meaning); }),
        "every dictionary meaning is in the string pool");
  check(every<rel::FallbackRowRecord>(rel::Section::FallbackRows,
                                      [&](const rel::FallbackRowRecord& r) {
                                        return r.inflect < count(rel::Section::Inflections) &&
                                               r.description < count(rel::Section::Grammars);
                                      }),
        "every fallback row names an inflection and a grammar");
  check(every<rel::ClassRecord>(
            rel::Section::Classes,
            [&](const rel::ClassRecord& r) { return r.rows().end() <= count(rel::Section::FallbackRows); }),
        "every class's rows are fallback rows");
  check(
      every<rel::InflectionRecord>(rel::Section::Inflections,
                                   [&](const rel::InflectionRecord& r) { return inPool(r.ending) && r.stemKey <= 9; }),
      "every inflection's ending and stem key are valid");
  std::string_view previous;
  check(every<rel::EndingRecord>(rel::Section::Endings,
                                 [&](const rel::EndingRecord& r) {
                                   if (!inPool(r.text) || r.inflections().end() > count(rel::Section::Inflections) ||
                                       (previous.data() != nullptr && !(previous < text(r.text))))
                                     return false;
                                   previous = text(r.text);
                                   return true;
                                 }),
        "the endings are sorted and name inflections");
  previous = {};
  check(every<rel::FallbackStemRecord>(rel::Section::FallbackStems,
                                       [&](const rel::FallbackStemRecord& r) {
                                         if (!inPool(r.text) || r.dictionary >= count(rel::Section::Dictionaries) ||
                                             r.stemKey > 9 || !part(r.part) || r.part == latin::Part::X ||
                                             (previous.data() != nullptr && text(r.text) < previous))
                                           return false;
                                         previous = text(r.text);
                                         return true;
                                       }),
        "the fallback stems are sorted and name dictionary entries");
  check(every<rel::AddonRecord>(rel::Section::Addons,
                                [&](const rel::AddonRecord& r) {
                                  return inPool(r.fix) && inPool(r.meaning) &&
                                         latin::isAddonKind(std::to_underlying(r.kind)) && part(r.root) &&
                                         part(r.targetPart) && r.rootStemKey <= 9 && r.targetStemKey <= 9 &&
                                         r.rows().end() <= count(rel::Section::FallbackRows);
                                }),
        "every addon's strings, parts, keys and rows are valid");
}

} // namespace

void emitter::validateImage(const std::filesystem::path& path) {
  const std::vector<char> source = util::fileLoad(path);
  std::string failure;
  if (!rel::loadImage(std::as_bytes(std::span{source}), failure))
    error::fatal(failure);

  const std::vector<std::string> spellings = rel::spellings();
  validateSemanticRows(spellings);
  validateEntries();
  validateFallbackRecords();
  std::println("  exhaustive spelling / ordered-row / field check: exact");
}
