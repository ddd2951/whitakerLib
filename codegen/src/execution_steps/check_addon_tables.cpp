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
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using helpers::check;

[[nodiscard]] std::uint64_t count(const rel::Section section) { return rel::section(section).count; }

template <typename Record> [[nodiscard]] bool every(const rel::Section section, const auto& fits) {
  for (std::uint64_t i = 0; i < count(section); ++i)
    if (!fits(rel::readRecord<Record>(rel::recordAt(*rel::sImage, section, i))))
      return false;
  return true;
}

// INFO: The records only the addon search reads, which checkImageReadings' walk never reaches: each stays inside its
//  tables, and the two tables it searches are sorted.
void validateAddonSearchRecords() {
  const std::uint64_t pool = rel::section(rel::Section::Strings).bytes;
  const auto inPool = [pool](const std::uint32_t offset) { return offset < pool; };
  const auto text = [](const std::uint32_t offset) { return std::string_view{rel::sImage->strings + offset}; };
  const auto part = [](const latin::Part value) { return latin::isPart(std::to_underlying(value)); };

  check(pool != 0 && rel::sImage->strings[pool - 1] == '\0', "the string pool ends with a terminator");
  check(every<rel::DictionaryRecord>(rel::Section::Dictionaries,
                                     [&](const rel::DictionaryRecord& r) { return inPool(r.meaning); }),
        "every dictionary meaning is in the string pool");
  check(every<rel::ClassRowRecord>(rel::Section::ClassRows,
                                   [&](const rel::ClassRowRecord& r) {
                                     return r.inflect < count(rel::Section::Inflections) &&
                                            r.description < count(rel::Section::Grammars);
                                   }),
        "every class row names an inflection and a grammar");
  check(every<rel::ClassRecord>(
            rel::Section::Classes,
            [&](const rel::ClassRecord& r) { return rel::end(rel::rows(r)) <= count(rel::Section::ClassRows); }),
        "every class's rows are class rows");
  check(
      every<rel::InflectionRecord>(rel::Section::Inflections,
                                   [&](const rel::InflectionRecord& r) { return inPool(r.ending) && r.stemKey <= 9; }),
      "every inflection's ending and stem key are valid");
  std::string_view previous{};
  check(every<rel::EndingRecord>(rel::Section::Endings,
                                 [&](const rel::EndingRecord& r) {
                                   if (!inPool(r.text) ||
                                       rel::end(rel::inflections(r)) > count(rel::Section::Inflections) ||
                                       (previous.data() != nullptr && !(previous < text(r.text))))
                                     return false;
                                   previous = text(r.text);
                                   return true;
                                 }),
        "the endings are sorted and name inflections");
  std::pair<std::uint16_t, std::string_view> before{};
  bool first = true;
  check(every<rel::InflectionLabelRecord>(
            rel::Section::InflectionLabels,
            [&](const rel::InflectionLabelRecord& r) {
              const std::pair<std::uint16_t, std::string_view> key{r.description, text(r.ending)};
              if (!inPool(r.ending) || r.description >= count(rel::Section::Grammars) || (!first && !(before < key)))
                return false;
              before = key, first = false;
              return true;
            }),
        "the inflection labels are sorted and name grammars");
  previous = {};
  check(every<rel::StemRecord>(rel::Section::Stems,
                               [&](const rel::StemRecord& r) {
                                 return r.dictionary < count(rel::Section::Dictionaries) &&
                                        (r.stemKey <= 9 || r.stemKey == 255) && part(rel::part(r)) &&
                                        rel::part(r) != latin::Part::X;
                               }),
        "the stems name dictionary entries");
  check(every<rel::AddonRecord>(rel::Section::Addons,
                                [&](const rel::AddonRecord& r) {
                                  return inPool(r.fix) && inPool(r.meaning) &&
                                         latin::isAddonKind(std::to_underlying(r.kind)) && part(r.root) &&
                                         part(r.targetPart) && r.rootStemKey <= 9 && r.targetStemKey <= 9 &&
                                         rel::end(rel::rows(r)) <= count(rel::Section::ClassRows);
                                }),
        "every addon's strings, parts, keys and rows are valid");
}

} // namespace

void execution_steps::image_file::checkAddonTables() {
  validateAddonSearchRecords();
  std::println("  exhaustive spelling / ordered-row / field check: exact");
}
