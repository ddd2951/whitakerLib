#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "error/error.hpp"
#include "expand/expand.hpp"
#include "facts.hpp"
#include "helpers/description_id.hpp"
#include "source/source.hpp"
#include "storage/description_ids.hpp"
#include "storage/image.hpp"
#include "storage/source.hpp"
#include "util/reflect_util.hpp"
#include "word/word.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <map>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using namespace emitter;
using helpers::descriptionId;
using storage::DescriptionIds;
using storage::SourceRow;

struct FormAddress {
  std::uint32_t dictionary{};
  std::uint8_t column{};
};
static_assert(std::is_trivially_copyable_v<FormAddress>);

[[nodiscard]] FormAddress addressOf(const expand::Origin& origin) {
  constexpr std::uint32_t uniqueBase = word::kDictlineWords;
  return std::visit(
      []<typename T>(const T& value) -> FormAddress {
        if constexpr (std::is_same_v<T, expand::Joined>)
          return {value.entry.value, value.column.value};
        else
          return {uniqueBase + value.entry.value, 1};
      },
      origin);
}

[[nodiscard]] std::string dictlineLine(const expand::DictlineRow row) {
  if (row.value >= source::scheme::dictline::kEntriesPerFile)
    return std::format("    DICTLINE synthetic row {}\n", row.value);
  const source::DictlineIndex index{row.value};
  return std::format("    DICTLINE.GEN:{}  {}\n", source::lineNumber(index), source::dictline(index));
}

[[nodiscard]] std::string inflectsLine(const expand::InflectsRow row) {
  if (row.value >= source::scheme::inflects::kEntriesPerFile)
    return std::format("    INFLECTS synthetic row {}\n", row.value);
  const source::InflectsIndex index{row.value};
  return std::format("    INFLECTS.LAT:{}  {}\n", source::lineNumber(index), source::inflects(index));
}

[[nodiscard]] std::string uniquesLines(const expand::UniquesRow row) {
  const source::UniquesIndex index{row.value};
  std::string out = std::format("    UNIQUES.LAT:{}\n", source::lineNumber(index));
  for (const std::string_view line : source::uniques(index))
    out += std::format("      {}\n", line);
  return out;
}

[[nodiscard]] std::string sourceLines(const expand::Origin& origin) {
  return std::visit(
      []<typename T>(const T& value) {
        if constexpr (std::is_same_v<T, expand::Joined>)
          return dictlineLine(value.entry) + inflectsLine(value.inflection);
        else if constexpr (std::is_same_v<T, expand::Unique>)
          return uniquesLines(value.entry);
        else
          return uniquesLines(value.entry) + inflectsLine(value.inflection);
      },
      origin);
}

[[nodiscard]] std::string resultReport(const std::size_t index) {
  const expand::ResultIndex result{index};
  const expand::Origin origin = expand::origin(result);
  return std::format("  result {} spelling \"{}\"\n  origin {}\n{}"
                     "  reading {}\n",
                     index, expand::spelling(result), util::describe(origin), sourceLines(origin),
                     util::describe(expand::reading(result)));
}

[[noreturn]] void conflict(const std::string_view what, const std::string_view changed, const std::size_t first,
                           const std::size_t second,
                           const std::source_location location = std::source_location::current()) {
  error::fatal(
      std::format("image: {}\n{}first:\n{}second:\n{}", what, changed, resultReport(first), resultReport(second)),
      location);
}

struct EntryFields {
  std::string_view meaning{};
  latin::Age age{};
  latin::Frequency frequency{};
  friend bool operator==(const EntryFields&, const EntryFields&) = default;
};
static_assert(std::is_trivially_copyable_v<EntryFields>);

using StemPatternIds = std::map<std::string, std::uint16_t>;

// NOTE: Pattern 0 is the empty one: the stem spells as it folds.
[[nodiscard]] std::uint16_t printedStem(const std::string_view orth, ImageData& image, StemPatternIds& ids,
                                        const std::size_t index) {
  std::string marks{};
  bool marked = false;
  for (const char c : orth) {
    const char folded = facts::foldLetter(c);
    marks += folded == c ? '.' : c;
    marked = marked || folded != c;
  }
  if (!marked)
    marks.clear();
  const auto [found, inserted] = ids.try_emplace(marks, static_cast<std::uint16_t>(ids.size()));
  if (inserted)
    image.stemPatterns.push_back(image.strings.intern(marks));
  if (orth.size() >= 1u << rel::kStemLengthBits || found->second >= 1u << (16 - rel::kStemLengthBits))
    error::fatal(std::format("image: printed stem \"{}\" does not fit its 16 bits\n{}", orth, resultReport(index)));
  return static_cast<std::uint16_t>(std::size_t{found->second} << rel::kStemLengthBits | orth.size());
}

[[nodiscard]] std::vector<SourceRow> collectReadings(ImageData& image, DescriptionIds& descriptionIds) {
  using FirstResult = std::optional<std::size_t>;
  std::vector<std::array<FirstResult, facts::kStemColumnCount>> orthFirst(image.dictionaries.size());
  std::vector<FirstResult> entryFirst(image.dictionaries.size());
  std::vector<EntryFields> entryFields(image.dictionaries.size());
  std::vector<SourceRow> source{};
  source.reserve(expand::resultCount());
  StemPatternIds stemPatternIds{{"", std::uint16_t{0}}};
  image.stemPatterns.push_back(image.strings.intern(""));
  // INFO: The inflection's labels by grammar and ending. Five pairs have two INFLECTS frequencies (A or B, which WORDS
  //  never shows); the first reading's is kept.
  std::map<std::pair<std::uint16_t, std::string>, std::pair<latin::Age, latin::Frequency>> labels{};

  for (std::size_t index = 0; index < expand::resultCount(); ++index) {
    const expand::ResultIndex result{index};
    if (expand::fate(result) != expand::Fate::kept)
      continue;
    const std::string_view spelling = expand::spelling(result);
    const FormAddress address = addressOf(expand::origin(result));
    if (spelling.size() > facts::kMaxWordCharacters)
      error::fatal(std::format("image: a generated form of {} letters exceeds the image's {}\n{}", spelling.size(),
                               facts::kMaxWordCharacters, resultReport(index)));
    if (address.dictionary >= image.dictionaries.size() || address.column < 1 ||
        address.column > facts::kStemColumnCount)
      error::fatal(std::format("image: address {} is out of bounds ({} dictionaries, {} stem columns)\n{}",
                               util::describe(address), image.dictionaries.size(), facts::kStemColumnCount,
                               resultReport(index)));
    const expand::Reading reading = expand::reading(result);
    const Rendered rendered = render(reading);
    const std::uint16_t stem = printedStem(rendered.orth, image, stemPatternIds, index);

    rel::DictionaryRecord& dictionary = image.dictionaries[address.dictionary];
    const std::size_t slot = std::size_t{address.column} - 1;
    FirstResult& orthSeen = orthFirst[address.dictionary][slot];
    if (!orthSeen) {
      dictionary.stems[slot] = stem;
      orthSeen = index;
    } else if (dictionary.stems[slot] != stem) {
      const std::string_view before = render(expand::reading(expand::ResultIndex{*orthSeen})).orth;
      conflict("one dictionary stem renders two orthographies",
               std::format("  orth: \"{}\" -> \"{}\"\n", before, rendered.orth), *orthSeen, index);
    }

    const EntryFields fields{rendered.meaning, reading.entryAge, reading.entryFrequency};
    FirstResult& entrySeen = entryFirst[address.dictionary];
    EntryFields& first = entryFields[address.dictionary];
    if (!entrySeen) {
      dictionary.meaning = image.strings.intern(fields.meaning);
      dictionary.age = fields.age;
      dictionary.frequency = fields.frequency;
      entrySeen = index;
      first = fields;
    } else if (first != fields) {
      conflict("one dictionary entry renders two sets of entry fields", util::differences(first, fields), *entrySeen,
               index);
    }

    const Analysis analysis{static_cast<std::uint16_t>(address.dictionary), static_cast<std::uint8_t>(slot),
                            descriptionId(rendered.analysis, image, descriptionIds)};
    source.push_back(SourceRow{normalize(spelling), analysis});
    labels.try_emplace({analysis.description, source.back().word.substr(rendered.orth.size())}, reading.inflectionAge,
                       reading.inflectionFrequency);
  }
  // INFO: Try_Tackons sweeps only what a tackon accepts, and the regular tackons take PRON and ADJ: those readings
  //  List_Sweep trimmed from a spelling alone are kept apart (familias' N is left out). Stems, descriptions and labels
  //  only where no kept reading set them.
  for (std::size_t index = 0; index < expand::resultCount(); ++index) {
    const expand::ResultIndex result{index};
    const expand::Fate fate = expand::fate(result);
    if (fate == expand::Fate::kept || fate == expand::Fate::duplicate)
      continue;
    const expand::Reading reading = expand::reading(result);
    const Rendered rendered = render(reading);
    if (rendered.analysis.part != latin::Part::PRON && rendered.analysis.part != latin::Part::ADJ)
      continue;
    const FormAddress address = addressOf(expand::origin(result));
    const std::size_t slot = std::size_t{address.column} - 1;
    rel::DictionaryRecord& dictionary = image.dictionaries[address.dictionary];
    const std::uint16_t stem = printedStem(rendered.orth, image, stemPatternIds, index);
    if (!orthFirst[address.dictionary][slot]) {
      dictionary.stems[slot] = stem;
      orthFirst[address.dictionary][slot] = index;
    } else if (dictionary.stems[slot] != stem) {
      conflict("a trimmed reading renders another orthography", std::format("  orth: \"{}\"\n", rendered.orth),
               *orthFirst[address.dictionary][slot], index);
    }
    if (!entryFirst[address.dictionary]) {
      dictionary.meaning = image.strings.intern(rendered.meaning);
      dictionary.age = reading.entryAge;
      dictionary.frequency = reading.entryFrequency;
      entryFirst[address.dictionary] = index;
    }
    const std::string word = normalize(expand::spelling(result));
    const std::uint16_t description = descriptionId(rendered.analysis, image, descriptionIds);
    labels.try_emplace({description, word.substr(rendered.orth.size())}, reading.inflectionAge,
                       reading.inflectionFrequency);
    image.trimmedReadings.push_back(rel::TrimmedReadingRecord{
        .text = image.strings.intern(word),
        .dictionary = static_cast<std::uint16_t>(address.dictionary),
        .stem = static_cast<std::uint8_t>(slot),
        .kind = fate == expand::Fate::stemNotAllowed ? rel::kTrimmedNotAllowed : rel::kTrimmedRare,
        .description = description});
  }
  std::ranges::stable_sort(image.trimmedReadings,
                           [&image](const rel::TrimmedReadingRecord& a, const rel::TrimmedReadingRecord& b) {
                             return std::string_view(image.strings.blob().c_str() + a.text) <
                                    std::string_view(image.strings.blob().c_str() + b.text);
                           });
  for (const auto& [key, value] : labels)
    image.inflectionLabels.push_back(rel::InflectionLabelRecord{.ending = image.strings.intern(key.second),
                                                                .description = key.first,
                                                                .age = value.first,
                                                                .frequency = value.second});
  return source;
}

} // namespace

void execution_steps::image_data::collectReadings() {
  using ::collectReadings;
  using storage::descriptionIds;
  using storage::image;
  using storage::source;
  source = collectReadings(image, descriptionIds);
}
