#include "image_builder.hpp"

#include "entry_values.hpp"
#include "error/error.hpp"
#include "expand/common/licensing.hpp"
#include "expand/common/stem_column.hpp"
#include "expand/expand.hpp"
#include "facts.hpp"
#include "latin.hpp"
#include "rendered_analysis.hpp"
#include "source/source.hpp"
#include "src/search/relationship_schema.hpp"
#include "string_section.hpp"
#include "types/grammar.hpp"
#include "util/reflect_util.hpp"
#include "word/word.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <source_location>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace rel = whitaker::relationship;

namespace emitter {

namespace {

struct SourceRow {
  std::string word;
  Analysis analysis;
};

template <typename Value> [[nodiscard]] std::uint16_t addonValue(Value value) noexcept {
  if constexpr (requires { value.value; })
    return value.value;
  else
    return static_cast<std::uint16_t>(value);
}

[[nodiscard]] std::uint8_t firstLetter(std::string_view fix) noexcept {
  return static_cast<std::uint8_t>(fix.empty() ? '\0' : fix.front());
}

[[nodiscard]] rel::AddonRecord addonRecord(const latin::addon::Prefix& prefix, std::string_view meaning,
                                           latin::AddonKind kind, StringSection& strings) {
  return rel::AddonRecord{
      .fix = strings.intern(normalize(prefix.fix)),
      .meaning = strings.intern(meaning),
      .kind = kind,
      .root = prefix.from,
      .targetPart = prefix.to,
      .connectingLetter = static_cast<std::uint8_t>(prefix.connect),
      .firstLetterAsSpelled = firstLetter(prefix.fix),
  };
}

void writeAddonTarget(rel::AddonRecord& result, const latin::addon::Target& value) {
  result.targetPart = latin::partOf(value);
  std::visit(
      [&result](const auto& target) {
        using Target = std::remove_cvref_t<decltype(target)>;
        if constexpr (std::is_same_v<Target, latin::Noun>) {
          result.target = {addonValue(target.declension), addonValue(target.declensionVariant),
                           addonValue(target.gender), addonValue(target.nounKind)};
        } else if constexpr (std::is_same_v<Target, latin::Pronoun>) {
          result.target = {addonValue(target.declension), addonValue(target.declensionVariant),
                           addonValue(target.pronounKind), 0};
        } else if constexpr (std::is_same_v<Target, latin::Packon>) {
          result.target = {addonValue(target.declension), addonValue(target.declensionVariant),
                           addonValue(target.packonKind), 0};
        } else if constexpr (std::is_same_v<Target, latin::Adjective>) {
          result.target = {addonValue(target.declension), addonValue(target.declensionVariant),
                           addonValue(target.comparison), 0};
        } else if constexpr (std::is_same_v<Target, latin::Numeral>) {
          result.target = {addonValue(target.declension), addonValue(target.declensionVariant),
                           addonValue(target.numeralSort), addonValue(target.numeralValue)};
        } else if constexpr (std::is_same_v<Target, latin::Adverb>) {
          result.target = {addonValue(target.comparison), 0, 0, 0};
        } else if constexpr (std::is_same_v<Target, latin::Verb>) {
          result.target = {addonValue(target.conjugation), addonValue(target.conjugationVariant),
                           addonValue(target.verbKind), 0};
        }
      },
      value);
}

[[nodiscard]] rel::AddonRecord addonRecord(const latin::addon::Suffix& addon, std::string_view meaning,
                                           latin::AddonKind kind, StringSection& strings) {
  rel::AddonRecord result{
      .fix = strings.intern(normalize(addon.fix)),
      .meaning = strings.intern(meaning),
      .kind = kind,
      .root = addon.from,
      .rootStemKey = addon.fromKey.value,
      .targetStemKey = addon.toKey.value,
      .connectingLetter = static_cast<std::uint8_t>(addon.connect),
      .firstLetterAsSpelled = firstLetter(addon.fix),
  };
  writeAddonTarget(result, addon.grammar);
  return result;
}

[[nodiscard]] rel::AddonRecord addonRecord(const latin::addon::Tackon& tackon, std::string_view meaning,
                                           latin::AddonKind kind, StringSection& strings) {
  rel::AddonRecord result{
      .fix = strings.intern(normalize(tackon.fix)),
      .meaning = strings.intern(meaning),
      .kind = kind,
      .root = latin::Part::X,
      .firstLetterAsSpelled = firstLetter(tackon.fix),
  };
  writeAddonTarget(result, tackon.grammar);
  return result;
}

[[nodiscard]] std::optional<latin::Entry> addonGrammar(const latin::addon::Target& value) {
  return std::visit(
      [](const auto& target) -> std::optional<latin::Entry> {
        using Target = std::remove_cvref_t<decltype(target)>;
        if constexpr (std::is_same_v<Target, latin::addon::Any> || std::is_same_v<Target, latin::Packon>)
          return std::nullopt;
        else
          return latin::Entry{target};
      },
      value);
}

using GrammarKey = std::array<std::uint16_t, 4>;

// NOTE: Entries with the same key share one class, so the key holds only what the join reads. Verb kind goes on each
//  entry instead.
[[nodiscard]] GrammarKey grammarKey(const latin::Entry& grammar, std::uint16_t packon) {
  const latin::Part part = partOf(grammar);
  GrammarKey key{static_cast<std::uint16_t>(part), 0, 0, packon};
  std::visit(
      [&key](const auto& value) {
        using Grammar = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::is_same_v<Grammar, latin::Noun>)
          key = {key[0], addonValue(value.declension), addonValue(value.declensionVariant), addonValue(value.gender)};
        else if constexpr (std::is_same_v<Grammar, latin::Pronoun> || std::is_same_v<Grammar, latin::Packon>)
          key = {key[0], addonValue(value.declension), addonValue(value.declensionVariant), key[3]};
        else if constexpr (std::is_same_v<Grammar, latin::Verb>)
          key = {key[0], addonValue(value.conjugation), addonValue(value.conjugationVariant), 0};
        else if constexpr (std::is_same_v<Grammar, latin::Adjective>)
          key = {key[0], addonValue(value.declension), addonValue(value.declensionVariant),
                 addonValue(value.comparison)};
        else if constexpr (std::is_same_v<Grammar, latin::Numeral>)
          key = {key[0], addonValue(value.declension), addonValue(value.declensionVariant),
                 addonValue(value.numeralSort)};
        else if constexpr (std::is_same_v<Grammar, latin::Adverb>)
          key = {key[0], addonValue(value.comparison), 0, 0};
        else if constexpr (std::is_same_v<Grammar, latin::Preposition>)
          key = {key[0], addonValue(value.caseOf), 0, 0};
      },
      grammar);
  return key;
}

[[nodiscard]] latin::VerbKind entryVerbKind(const latin::Entry& grammar) noexcept {
  if (const auto* verb = std::get_if<latin::Verb>(&grammar))
    return verb->verbKind;
  return latin::VerbKind::X;
}

[[nodiscard]] std::uint16_t grammarRules(const latin::Entry& grammar, std::uint16_t packon) noexcept {
  const latin::Part part = partOf(grammar);
  std::uint16_t rules = static_cast<std::uint16_t>(packon << rel::kRulePackonShift);
  if (part == latin::Part::CONJ || part == latin::Part::INTERJ)
    rules |= rel::kRuleInterjOrConj;
  std::visit(
      [&rules](const auto& value) {
        using Grammar = std::remove_cvref_t<decltype(value)>;
        if constexpr (requires { value.declension; }) {
          if (value.declension.value == 9 && value.declensionVariant.value == 8)
            rules |= rel::kRuleAbbreviation;
        }
        if constexpr (std::is_same_v<Grammar, latin::Verb>) {
          if (value.conjugation.value == 3 && value.conjugationVariant.value == 1)
            rules |= rel::kRuleConjThreeOne;
        }
      },
      grammar);
  return rules;
}

[[nodiscard]] std::uint8_t inflectionAllow(const word::InflectsEntry& inflection) noexcept {
  const auto* verb = std::get_if<latin::inflected::Verb>(&inflection.grammar);
  if (verb == nullptr)
    return 0;
  return latin::allowOf(verb->tense, verb->voice, verb->mood, verb->person, verb->number,
                        inflection.characterCount.value == 0)
      .bits;
}

using AddonRows = std::array<std::vector<std::size_t>, util::kEnumerators<latin::AddonKind>.size()>;

[[nodiscard]] const word::AddonsEntry& addonAt(std::size_t row) { return word::entry(source::AddonsIndex{row}); }

[[nodiscard]] AddonRows classifyAddons() {
  AddonRows result;
  for (std::size_t row = 0; row < source::scheme::addons::kEntriesPerFile; ++row)
    result[std::to_underlying(addonAt(row).kind)].push_back(row);
  return result;
}

static_assert(word::kDictlineWords + source::scheme::uniques::kEntriesPerFile <=
              std::size_t{std::numeric_limits<std::uint16_t>::max()} + 1);

struct FormAddress {
  std::uint32_t dictionary{};
  std::uint8_t column{};
};

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

[[nodiscard]] std::string dictlineLine(expand::DictlineRow row) {
  if (row.value >= source::scheme::dictline::kEntriesPerFile)
    return std::format("    DICTLINE synthetic row {}\n", row.value);
  const source::DictlineIndex index{row.value};
  return std::format("    DICTLINE.GEN:{}  {}\n", source::lineNumber(index), source::dictline(index));
}

[[nodiscard]] std::string inflectsLine(expand::InflectsRow row) {
  if (row.value >= source::scheme::inflects::kEntriesPerFile)
    return std::format("    INFLECTS synthetic row {}\n", row.value);
  const source::InflectsIndex index{row.value};
  return std::format("    INFLECTS.LAT:{}  {}\n", source::lineNumber(index), source::inflects(index));
}

[[nodiscard]] std::string uniquesLines(expand::UniquesRow row) {
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

[[nodiscard]] std::string resultReport(std::size_t index) {
  const expand::ResultIndex result{index};
  const expand::Origin origin = expand::origin(result);
  return std::format("  result {} spelling \"{}\"\n  origin {}\n{}"
                     "  reading {}\n",
                     index, expand::spelling(result), util::describe(origin), sourceLines(origin),
                     util::describe(expand::reading(result)));
}

[[noreturn]] void conflict(std::string_view what, std::string_view changed, std::size_t first, std::size_t second,
                           std::source_location location = std::source_location::current()) {
  error::fatal(
      std::format("image: {}\n{}first:\n{}second:\n{}", what, changed, resultReport(first), resultReport(second)),
      location);
}

struct EntryFields {
  std::string_view meaning;
  latin::Age age;
  latin::Frequency frequency;
  friend bool operator==(const EntryFields&, const EntryFields&) = default;
};

using StemPatternIds = std::map<std::string, std::uint16_t>;

// NOTE: Pattern 0 is the empty one: the stem spells as it folds.
[[nodiscard]] std::uint16_t printedStem(std::string_view orth, ImageData& image, StemPatternIds& ids,
                                        std::size_t index) {
  std::string marks;
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

using DescriptionIds = std::map<std::string, std::uint32_t>;
using EndingOrder = std::vector<std::pair<std::string, std::uint32_t>>;

[[nodiscard]] std::uint16_t descriptionId(const latin::Analysis& analysis, ImageData& image,
                                          DescriptionIds& descriptionIds) {
  auto [description, inserted] =
      descriptionIds.emplace(describe(analysis), static_cast<std::uint32_t>(image.grammars.size()));
  if (inserted) {
    const rel::UnpackedGrammar back = rel::unpackGrammar(rel::packGrammar(analysis));
    if (!back.valid || back.analysis != analysis)
      error::fatal(std::format("image: description \"{}\" has a field wider than its grammar bits\n{}",
                               description->first, util::describe(analysis)));
    image.grammars.push_back(analysis);
  } else if (image.grammars[description->second] != analysis) {
    error::fatal(std::format("image: description \"{}\" renders two grammars\n{}", description->first,
                             util::differences(image.grammars[description->second], analysis)));
  }
  if (description->second > std::numeric_limits<std::uint16_t>::max())
    error::fatal("image: more descriptions than 16 bits can name");
  return static_cast<std::uint16_t>(description->second);
}

void fallbackRow(const latin::Entry& grammar, const word::InflectsEntry& inflection, std::uint32_t position,
                 ImageData& image, DescriptionIds& descriptionIds) {
  const latin::Analysis analysis = analysisOf(grammar, inflection.grammar, inflection.stemKey);
  const std::uint16_t description = descriptionId(analysis, image, descriptionIds);
  image.fallbackRows.push_back(rel::FallbackRowRecord{static_cast<std::uint16_t>(position), description});
}

std::size_t internAddons(const AddonRows& addonRows, ImageData& image) {
  for (const std::vector<std::size_t>& rows : addonRows)
    for (const std::size_t row : rows) {
      const word::AddonsEntry& addon = addonAt(row);
      image.addons.push_back(
          std::visit([&](const auto& value) { return addonRecord(value, addon.meaning, addon.kind, image.strings); },
                     addon.addon));
    }
  return addonRows[std::to_underlying(latin::AddonKind::Tickon)].size() +
         addonRows[std::to_underlying(latin::AddonKind::Prefix)].size();
}

// NOTE: Ordinals start at 1 so that an entry naming no packon reads as 0.
[[nodiscard]] std::map<std::string, std::uint16_t> packonOrdinals(const AddonRows& addonRows) {
  const std::vector<std::size_t>& packons = addonRows[std::to_underlying(latin::AddonKind::Packon)];
  std::map<std::string, std::uint16_t> result;
  for (std::size_t index = 0; index < packons.size(); ++index) {
    const std::uint16_t ordinal = static_cast<std::uint16_t>(index + 1);
    if (ordinal > rel::kRulePackonMask)
      error::fatal("image: more packons than the class rules can name");
    const auto packon = std::get<latin::addon::Tackon>(addonAt(packons[index]).addon);
    result.emplace(normalize(packon.fix), ordinal);
  }
  return result;
}

void internEntries(ImageData& image) {
  image.dictionaries.resize(word::kDictlineWords + source::scheme::uniques::kEntriesPerFile);
  image.entries.reserve(image.dictionaries.size());
  for (std::uint32_t index = 0; index < word::kDictlineWords; ++index)
    image.entries.push_back(entryValues(word::entry(word::DictlineRow{index})));
  for (std::uint32_t index = 0; index < source::scheme::uniques::kEntriesPerFile; ++index)
    image.entries.push_back(entryValues(word::entry(word::UniquesRow{index})));
}

[[nodiscard]] std::vector<latin::Entry> internClasses(const std::map<std::string, std::uint16_t>& packons,
                                                      ImageData& image) {
  std::map<GrammarKey, std::uint16_t> classIds;
  std::vector<latin::Entry> classGrammars;
  for (std::uint32_t index = 0; index < source::scheme::dictline::kEntriesPerFile; ++index) {
    const word::DictlineEntry& dictionary = word::entry(word::DictlineRow{index});
    std::uint16_t packon = 0;
    if (partOf(dictionary.grammar) == latin::Part::PACK) {
      const auto named = packons.find(normalize(dictionary.packon));
      if (named != packons.end()) {
        packon = named->second;
        // NOTE: A PACK entry produces no form, so its meaning is interned here.
        image.dictionaries[index].meaning = image.strings.intern(dictionary.senses);
      }
    }
    auto [entry, inserted] =
        classIds.emplace(grammarKey(dictionary.grammar, packon), static_cast<std::uint16_t>(classGrammars.size()));
    if (inserted) {
      if (entry->second > rel::kClassIdMask)
        error::fatal("image: more classes than a dictionary's class id can name");
      classGrammars.push_back(dictionary.grammar);
      image.classes.push_back(rel::ClassRecord{.rules = grammarRules(dictionary.grammar, packon)});
    }
    image.dictionaries[index].age = dictionary.labels.age;
    image.dictionaries[index].frequency = dictionary.labels.frequency;
    image.dictionaries[index].classId =
        entry->second |
        static_cast<std::uint16_t>(std::to_underlying(entryVerbKind(dictionary.grammar)) << rel::kClassVerbKindShift);
  }
  return classGrammars;
}

[[nodiscard]] std::vector<SourceRow> collectReadings(ImageData& image, DescriptionIds& descriptionIds) {
  using FirstResult = std::optional<std::size_t>;
  std::vector<std::array<FirstResult, facts::kStemColumnCount>> orthFirst(image.dictionaries.size());
  std::vector<FirstResult> entryFirst(image.dictionaries.size());
  std::vector<EntryFields> entryFields(image.dictionaries.size());
  std::vector<SourceRow> source;
  source.reserve(expand::resultCount());
  StemPatternIds stemPatternIds{{"", std::uint16_t{0}}};
  image.stemPatterns.push_back(image.strings.intern(""));

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
  }
  return source;
}

// NOTE: Rows keep order from expansion.
void groupSpellings(std::vector<SourceRow> source, ImageData& image) {
  std::ranges::stable_sort(source, {}, &SourceRow::word);
  image.words.reserve(source.size());
  image.analyses.reserve(source.size());
  for (std::size_t i = 0; i < source.size();) {
    const std::size_t begin = i;
    while (i < source.size() && source[i].word == source[begin].word)
      ++i;
    const std::size_t count = i - begin;
    if (count > rel::kMaximumResultsPerSpelling)
      error::fatal(std::format("image: a spelling with more than {} results", rel::kMaximumResultsPerSpelling));
    image.words.push_back(
        Word{source[begin].word, static_cast<std::uint32_t>(image.analyses.size()), static_cast<std::uint16_t>(count)});
    for (std::size_t row = begin; row < i; ++row)
      image.analyses.push_back(source[row].analysis);
  }
}

// NOTE: A suffix can reach a stem no inflection joined.
void listFallbackStems(ImageData& image) {
  for (std::uint32_t index = 0; index < source::scheme::dictline::kEntriesPerFile; ++index) {
    const word::DictlineEntry& entry = word::entry(word::DictlineRow{index});
    std::set<std::pair<std::string, std::uint8_t>> seen;
    for (std::uint8_t column = 1; column <= facts::kStemColumnCount; ++column) {
      const word::Stem text = expand::stemAt(entry, latin::StemKey{column});
      if (!text || text->empty())
        continue;
      const auto key = expand::internal::stemKeyOfColumn(entry, column);
      if (!key)
        continue;
      if (!seen.emplace(normalize(*text), key->value).second)
        continue;
      image.fallbackStems.push_back(rel::FallbackStemRecord{image.strings.intern(normalize(*text)),
                                                            static_cast<std::uint16_t>(index), key->value,
                                                            partOf(entry.grammar)});
    }
  }
  std::ranges::sort(image.fallbackStems,
                    [&image](const rel::FallbackStemRecord& left, const rel::FallbackStemRecord& right) {
                      const std::string_view a = image.strings.blob().c_str() + left.text;
                      const std::string_view b = image.strings.blob().c_str() + right.text;
                      return std::tie(a, left.dictionary, left.stemKey) < std::tie(b, right.dictionary, right.stemKey);
                    });
}

// INFO: Run_Inflections scans INFLECTS.LAT rows only, never the synthetic ones.
[[nodiscard]] EndingOrder orderInflections(ImageData& image) {
  EndingOrder endingOrder;
  endingOrder.reserve(source::scheme::inflects::kEntriesPerFile);
  for (std::uint32_t row = 0; row < source::scheme::inflects::kEntriesPerFile; ++row)
    endingOrder.emplace_back(normalize(word::entry(word::InflectsRow{row}).ending), row);
  std::ranges::sort(endingOrder);

  image.inflections.reserve(endingOrder.size());
  for (const auto& [ending, row] : endingOrder) {
    const word::InflectsEntry& inflection = word::entry(word::InflectsRow{row});
    image.inflections.push_back(rel::InflectionRecord{.ending = image.strings.intern(ending),
                                                      .stemKey = inflection.stemKey.value,
                                                      .allow = inflectionAllow(inflection),
                                                      .age = inflection.age,
                                                      .frequency = inflection.frequency});
  }
  for (std::uint32_t row = 0; row < endingOrder.size();) {
    const std::uint32_t begin = row;
    while (row < endingOrder.size() && endingOrder[row].first == endingOrder[begin].first)
      ++row;
    image.endings.push_back(rel::EndingRecord{image.inflections[begin].ending, static_cast<std::uint16_t>(begin),
                                              static_cast<std::uint16_t>(row - begin)});
  }
  return endingOrder;
}

// NOTE: For prefixes: they start from a stem.
//  The runtime checks the key.
void licenseClasses(const std::vector<latin::Entry>& classGrammars, const EndingOrder& endingOrder, ImageData& image,
                    DescriptionIds& descriptionIds) {
  for (std::uint16_t id = 0; id < classGrammars.size(); ++id) {
    const latin::Entry& grammar = classGrammars[id];
    rel::ClassRecord& record = image.classes[id];
    const bool packon =
        partOf(grammar) == latin::Part::PACK && ((record.rules >> rel::kRulePackonShift) & rel::kRulePackonMask) != 0;
    record.rowStart = static_cast<std::uint32_t>(image.fallbackRows.size());
    for (std::uint32_t position = 0; position < endingOrder.size(); ++position) {
      const word::InflectsEntry& inflection = word::entry(word::InflectsRow{endingOrder[position].second});
      if (!(packon ? expand::internal::packonLicenses(grammar, inflection.grammar)
                   : expand::internal::licenses(grammar, inflection.grammar)))
        continue;
      fallbackRow(grammar, inflection, position, image, descriptionIds);
    }
    record.rowCount = static_cast<std::uint16_t>(image.fallbackRows.size() - record.rowStart);
  }
}

void licenseSuffixes(const AddonRows& addonRows, std::size_t suffixBase, const EndingOrder& endingOrder,
                     ImageData& image, DescriptionIds& descriptionIds) {
  const std::vector<std::size_t>& suffixes = addonRows[std::to_underlying(latin::AddonKind::Suffix)];
  for (std::size_t index = 0; index < suffixes.size(); ++index) {
    const auto suffix = std::get<latin::addon::Suffix>(addonAt(suffixes[index]).addon);
    const std::optional<latin::Entry> target = addonGrammar(suffix.grammar);
    rel::AddonRecord& record = image.addons[suffixBase + index];
    if (!target)
      continue;
    const latin::Part targetPart = partOf(*target);
    record.targetRules = static_cast<std::uint8_t>(grammarRules(*target, 0));
    record.rowStart = static_cast<std::uint32_t>(image.fallbackRows.size());
    for (std::uint32_t position = 0; position < endingOrder.size(); ++position) {
      const word::InflectsEntry& inflection = word::entry(word::InflectsRow{endingOrder[position].second});
      if (!expand::internal::licenses(*target, inflection.grammar))
        continue;
      if (!latin::stemKeyAdmits(record.targetStemKey, inflection.stemKey.value, targetPart))
        continue;
      fallbackRow(*target, inflection, position, image, descriptionIds);
    }
    record.rowCount = static_cast<std::uint16_t>(image.fallbackRows.size() - record.rowStart);
  }
}

} // namespace

[[nodiscard]] ImageData buildImageData() {
  ImageData image;
  const AddonRows addonRows = classifyAddons();
  const std::size_t suffixBase = internAddons(addonRows, image);
  internEntries(image);
  const std::vector<latin::Entry> classGrammars = internClasses(packonOrdinals(addonRows), image);
  DescriptionIds descriptionIds;
  groupSpellings(collectReadings(image, descriptionIds), image);
  listFallbackStems(image);
  const EndingOrder endingOrder = orderInflections(image);
  licenseClasses(classGrammars, endingOrder, image, descriptionIds);
  licenseSuffixes(addonRows, suffixBase, endingOrder, image, descriptionIds);
  image.program = build(image.words, image.analyses);
  return image;
}

} // namespace emitter
