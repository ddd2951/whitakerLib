#include "image_builder.hpp"

#include "error/error.hpp"
#include "entry_values.hpp"
#include "facts.hpp"
#include "rendered_analysis.hpp"
#include "expand/common/licensing.hpp"
#include "expand/common/rows.hpp"
#include "expand/common/stem_column.hpp"
#include "expand/expand.hpp"
#include "src/search/relationship_schema.hpp"
#include "string_section.hpp"
#include "types/grammar.hpp"
#include "latin.hpp"
#include "word/word.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace rel = whitaker::relationship;

namespace emitter {

namespace {

static_assert(facts::kMaxWordCharacters == rel::kMaximumWordLength,
              "the expansion's widest form and the relationship image's "
              "spelling capacity must agree");

struct SourceRow {
  std::string word;
  Analysis analysis;
  std::uint32_t sourceOrder{};
};

[[nodiscard]] latin::Part imageTargetPart(const latin::addon::Target& value) {
  return std::visit(
      [](const auto& target) -> latin::Part {
        using Target = std::remove_cvref_t<decltype(target)>;
        if constexpr (std::is_same_v<Target, latin::Noun>)
          return latin::Part::N;
        else if constexpr (std::is_same_v<Target, latin::Pronoun>)
          return latin::Part::PRON;
        else if constexpr (std::is_same_v<Target, latin::Packon>)
          return latin::Part::PACK;
        else if constexpr (std::is_same_v<Target, latin::Adjective>)
          return latin::Part::ADJ;
        else if constexpr (std::is_same_v<Target, latin::Numeral>)
          return latin::Part::NUM;
        else if constexpr (std::is_same_v<Target, latin::Adverb>)
          return latin::Part::ADV;
        else if constexpr (std::is_same_v<Target, latin::Verb>)
          return latin::Part::V;
        else
          return latin::Part::X;
      },
      value);
}

template <typename Value>
[[nodiscard]] std::uint16_t addonValue(Value value) noexcept {
  if constexpr (requires { value.value; })
    return value.value;
  else
    return static_cast<std::uint16_t>(value);
}

[[nodiscard]] AddonRecord makePrefixRecord(const latin::addon::Prefix& prefix,
                                           std::string_view meaning,
                                           latin::AddonKind kind,
                                           StringSection& strings) {
  return AddonRecord{
      .fix = strings.intern(normalize(prefix.fix)),
      .meaning = strings.intern(meaning),
      .kind = kind,
      .root = prefix.from,
      .targetPart = prefix.to,
      .connect = static_cast<std::uint8_t>(prefix.connect),
      .firstRaw = static_cast<std::uint8_t>(
          prefix.fix.empty() ? '\0' : prefix.fix.front()),
  };
}

void writeAddonTarget(AddonRecord& result, const latin::addon::Target& value) {
  result.targetPart = imageTargetPart(value);
  std::visit(
      [&result](const auto& target) {
        using Target = std::remove_cvref_t<decltype(target)>;
        if constexpr (std::is_same_v<Target, latin::Noun>) {
          result.target = {addonValue(target.declension),
                           addonValue(target.declensionVariant),
                           addonValue(target.gender),
                           addonValue(target.nounKind)};
        } else if constexpr (std::is_same_v<Target, latin::Pronoun>) {
          result.target = {addonValue(target.declension),
                           addonValue(target.declensionVariant),
                           addonValue(target.pronounKind), 0};
        } else if constexpr (std::is_same_v<Target, latin::Packon>) {
          result.target = {addonValue(target.declension),
                           addonValue(target.declensionVariant),
                           addonValue(target.packonKind), 0};
        } else if constexpr (std::is_same_v<Target, latin::Adjective>) {
          result.target = {addonValue(target.declension),
                           addonValue(target.declensionVariant),
                           addonValue(target.comparison), 0};
        } else if constexpr (std::is_same_v<Target, latin::Numeral>) {
          result.target = {addonValue(target.declension),
                           addonValue(target.declensionVariant),
                           addonValue(target.numeralSort),
                           addonValue(target.numeralValue)};
        } else if constexpr (std::is_same_v<Target, latin::Adverb>) {
          result.target = {addonValue(target.comparison), 0, 0, 0};
        } else if constexpr (std::is_same_v<Target, latin::Verb>) {
          result.target = {addonValue(target.conjugation),
                           addonValue(target.conjugationVariant),
                           addonValue(target.verbKind), 0};
        }
      },
      value);
}

[[nodiscard]] AddonRecord makeSuffixRecord(const latin::addon::Suffix& addon,
                                           std::string_view meaning,
                                           StringSection& strings) {
  AddonRecord result{
      .fix = strings.intern(normalize(addon.fix)),
      .meaning = strings.intern(meaning),
      .kind = latin::AddonKind::Suffix,
      .root = addon.from,
      .rootKey = addon.fromKey.value,
      .targetKey = addon.toKey.value,
      .connect = static_cast<std::uint8_t>(addon.connect),
      .firstRaw = static_cast<std::uint8_t>(
          addon.fix.empty() ? '\0' : addon.fix.front()),
  };
  writeAddonTarget(result, addon.grammar);
  return result;
}

[[nodiscard]] AddonRecord makeTackonRecord(const latin::addon::Tackon& tackon,
                                           std::string_view meaning,
                                           latin::AddonKind kind,
                                           StringSection& strings) {
  AddonRecord result{
      .fix = strings.intern(normalize(tackon.fix)),
      .meaning = strings.intern(meaning),
      .kind = kind,
      .root = latin::Part::X,
      .firstRaw = static_cast<std::uint8_t>(
          tackon.fix.empty() ? '\0' : tackon.fix.front()),
  };
  writeAddonTarget(result, tackon.grammar);
  return result;
}

[[nodiscard]] std::optional<latin::Entry>
addonGrammar(const latin::addon::Target& value) {
  return std::visit(
      [](const auto& target) -> std::optional<latin::Entry> {
        using Target = std::remove_cvref_t<decltype(target)>;
        if constexpr (std::is_same_v<Target, latin::addon::Any> ||
                      std::is_same_v<Target, latin::Packon>)
          return std::nullopt;
        else
          return latin::Entry{target};
      },
      value);
}

using GrammarKey = std::array<std::uint16_t, 4>;

// INFO: A class keys on the fields the join and parse line read; verb kind
//  travels on the entry, and a PACK entry's packon (from its gloss) keys.
[[nodiscard]] GrammarKey grammarKey(const latin::Entry& grammar,
                                    std::uint16_t packon) {
  const latin::Part part = partOf(grammar);
  GrammarKey key{static_cast<std::uint16_t>(part), 0, 0, packon};
  std::visit(
      [&key](const auto& value) {
        using Grammar = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::is_same_v<Grammar, latin::Noun>)
          key = {key[0], addonValue(value.declension),
                 addonValue(value.declensionVariant), addonValue(value.gender)};
        else if constexpr (std::is_same_v<Grammar, latin::Pronoun> ||
                           std::is_same_v<Grammar, latin::Packon>)
          key = {key[0], addonValue(value.declension),
                 addonValue(value.declensionVariant), key[3]};
        else if constexpr (std::is_same_v<Grammar, latin::Verb>)
          key = {key[0], addonValue(value.conjugation),
                 addonValue(value.conjugationVariant), 0};
        else if constexpr (std::is_same_v<Grammar, latin::Adjective>)
          key = {key[0], addonValue(value.declension),
                 addonValue(value.declensionVariant),
                 addonValue(value.comparison)};
        else if constexpr (std::is_same_v<Grammar, latin::Numeral>)
          key = {key[0], addonValue(value.declension),
                 addonValue(value.declensionVariant),
                 addonValue(value.numeralSort)};
        else if constexpr (std::is_same_v<Grammar, latin::Adverb>)
          key = {key[0], addonValue(value.comparison), 0, 0};
        else if constexpr (std::is_same_v<Grammar, latin::Preposition>)
          key = {key[0], addonValue(value.caseOf), 0, 0};
      },
      grammar);
  return key;
}

[[nodiscard]] latin::VerbKind
entryVerbKind(const latin::Entry& grammar) noexcept {
  if (const auto* verb = std::get_if<latin::Verb>(&grammar))
    return verb->verbKind;
  return latin::VerbKind::X;
}

[[nodiscard]] std::uint16_t grammarGate(const latin::Entry& grammar,
                                        std::uint16_t packon) noexcept {
  const latin::Part part = partOf(grammar);
  std::uint16_t gate =
      static_cast<std::uint16_t>(packon << rel::kGatePackonShift);
  if (part == latin::Part::CONJ || part == latin::Part::INTERJ)
    gate |= rel::kGateInterjOrConj;
  std::visit(
      [&gate](const auto& value) {
        using Grammar = std::remove_cvref_t<decltype(value)>;
        if constexpr (requires { value.declension; }) {
          if (value.declension.value == 9 && value.declensionVariant.value == 8)
            gate |= rel::kGateAbbreviation;
        }
        if constexpr (std::is_same_v<Grammar, latin::Verb>) {
          if (value.conjugation.value == 3 &&
              value.conjugationVariant.value == 1)
            gate |= rel::kGateConjThreeOne;
        }
      },
      grammar);
  return gate;
}

[[nodiscard]] std::uint8_t
inflectionAllow(const expand::InflectsEntry& inflection) noexcept {
  const auto* verb = std::get_if<latin::inflected::Verb>(&inflection.grammar);
  if (verb == nullptr)
    return 0;
  return latin::allowOf(verb->tense, verb->voice, verb->mood, verb->person,
                        verb->number, inflection.characterCount.value == 0)
      .bits;
}

struct AddonBuckets {
  std::vector<std::size_t> tickons;
  std::vector<std::size_t> prefixes;
  std::vector<std::size_t> suffixes;
  std::vector<std::size_t> tackons;
  std::vector<std::size_t> packons;
};

[[nodiscard]] word::Addons addonAt(std::size_t row) {
  return word::Addons{source::AddonsIndex{row}};
}

[[nodiscard]] AddonBuckets classifyAddons() {
  AddonBuckets result;
  for (std::size_t row = 0; row < source::scheme::addons::kEntriesPerFile;
       ++row) {
    switch (addonAt(row).kind()) {
    case latin::AddonKind::Tickon:
      result.tickons.push_back(row);
      break;
    case latin::AddonKind::Prefix:
      result.prefixes.push_back(row);
      break;
    case latin::AddonKind::Suffix:
      result.suffixes.push_back(row);
      break;
    case latin::AddonKind::Tackon:
      result.tackons.push_back(row);
      break;
    case latin::AddonKind::Packon:
      result.packons.push_back(row);
      break;
    }
  }
  return result;
}

struct FormAddress {
  std::uint32_t dictionary{};
  std::uint8_t stemSlot{};
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

using DescriptionIds = std::map<std::string, std::uint32_t>;
using EndingOrder = std::vector<std::pair<std::string, std::uint32_t>>;

[[nodiscard]] std::uint16_t descriptionId(const latin::Analysis& analysis,
                                          ImageData& image,
                                          DescriptionIds& descriptionIds) {
  auto [description, inserted] = descriptionIds.emplace(
      describe(analysis), static_cast<std::uint32_t>(image.grammars.size()));
  if (inserted) {
    image.grammars.push_back(analysis);
  } else if (image.grammars[description->second] != analysis) {
    error::fatal("relationship emitter: one description renders two "
                 "grammars");
  }
  if (description->second > std::numeric_limits<std::uint16_t>::max())
    error::fatal("relationship emitter: description exceeds the target's "
                 "16-bit capacity");
  return static_cast<std::uint16_t>(description->second);
}

void fallbackRow(const latin::Entry& grammar,
                 const expand::InflectsEntry& inflection,
                 std::uint32_t position, ImageData& image,
                 DescriptionIds& descriptionIds) {
  const latin::Analysis analysis =
      analysisOf(grammar, inflection.grammar, inflection.stemKey);
  const std::uint16_t description =
      descriptionId(analysis, image, descriptionIds);
  image.fallbackRows.push_back(
      FallbackRowRecord{static_cast<std::uint16_t>(position), description});
}

std::size_t internAddons(const AddonBuckets& addonRows, ImageData& image) {
  image.addons.reserve(addonRows.tickons.size() + addonRows.prefixes.size() +
                       addonRows.suffixes.size() + addonRows.tackons.size() +
                       addonRows.packons.size());
  for (const std::size_t row : addonRows.tickons)
    image.addons.push_back(makePrefixRecord(
        std::get<latin::addon::Prefix>(addonAt(row).addon()),
        addonAt(row).meaning(), latin::AddonKind::Tickon, image.strings));
  for (const std::size_t row : addonRows.prefixes)
    image.addons.push_back(makePrefixRecord(
        std::get<latin::addon::Prefix>(addonAt(row).addon()),
        addonAt(row).meaning(), latin::AddonKind::Prefix, image.strings));
  const std::size_t suffixBase = image.addons.size();
  for (const std::size_t row : addonRows.suffixes)
    image.addons.push_back(
        makeSuffixRecord(std::get<latin::addon::Suffix>(addonAt(row).addon()),
                         addonAt(row).meaning(), image.strings));
  for (const std::size_t row : addonRows.tackons)
    image.addons.push_back(makeTackonRecord(
        std::get<latin::addon::Tackon>(addonAt(row).addon()),
        addonAt(row).meaning(), latin::AddonKind::Tackon, image.strings));
  for (const std::size_t row : addonRows.packons)
    image.addons.push_back(makeTackonRecord(
        std::get<latin::addon::Tackon>(addonAt(row).addon()),
        addonAt(row).meaning(), latin::AddonKind::Packon, image.strings));
  return suffixBase;
}

// NOTE: Ordinals start at 1 so that an entry naming no packon reads as 0.
[[nodiscard]] std::map<std::string, std::uint16_t>
packonOrdinals(const AddonBuckets& addonRows) {
  std::map<std::string, std::uint16_t> result;
  for (std::size_t index = 0; index < addonRows.packons.size(); ++index) {
    const std::uint16_t ordinal = static_cast<std::uint16_t>(index + 1);
    if (ordinal > rel::kGatePackonMask)
      error::fatal("relationship emitter: more packons than the class gate "
                   "can name");
    const auto packon = std::get<latin::addon::Tackon>(
        addonAt(addonRows.packons[index]).addon());
    result.emplace(normalize(packon.fix), ordinal);
  }
  return result;
}

[[nodiscard]] std::vector<latin::Entry>
internClasses(const std::map<std::string, std::uint16_t>& packons,
              ImageData& image) {
  image.dictionaries.resize(word::kDictlineWords + expand::kUniquesRows);
  image.entries.reserve(image.dictionaries.size());
  for (std::uint32_t index = 0; index < word::kDictlineWords; ++index) {
    const word::Dictline word{word::DictlineRow{index}};
    image.entries.push_back(entryValues(word.grammar(), word.area(),
                                        word.geography(), word.source()));
  }
  for (std::uint32_t index = 0; index < expand::kUniquesRows; ++index) {
    const word::Uniques word{source::UniquesIndex{index}};
    image.entries.push_back(entryValues(word.grammar(), word.area(),
                                        word.geography(), word.source()));
  }
  std::map<GrammarKey, std::uint16_t> classIds;
  std::vector<latin::Entry> classGrammars;
  for (std::uint32_t index = 0; index < expand::kDictlineRows; ++index) {
    const expand::DictlineEntry dictionary =
        expand::entry(expand::DictlineRow{index});
    std::uint16_t packon = 0;
    if (partOf(dictionary.grammar) == latin::Part::PACK) {
      const auto named = packons.find(
          normalize(word::Dictline{word::DictlineRow{index}}.packon()));
      if (named != packons.end()) {
        packon = named->second;
        // NOTE: A PACK entry produces no form, so its meaning is interned here.
        image.dictionaries[index].meaning =
            image.strings.intern(dictionary.senses);
      }
    }
    auto [entry, inserted] =
        classIds.emplace(grammarKey(dictionary.grammar, packon),
                         static_cast<std::uint16_t>(classGrammars.size()));
    if (inserted) {
      if (entry->second > rel::kClassIdMask)
        error::fatal("relationship emitter: more classes than a dictionary's "
                     "class id can name");
      classGrammars.push_back(dictionary.grammar);
      image.classes.push_back(
          ClassRecord{.gate = grammarGate(dictionary.grammar, packon)});
    }
    image.dictionaries[index].age = std::to_underlying(dictionary.age);
    image.dictionaries[index].frequency =
        std::to_underlying(dictionary.frequency);
    image.dictionaries[index].classId =
        entry->second |
        static_cast<std::uint16_t>(
            std::to_underlying(entryVerbKind(dictionary.grammar))
            << rel::kClassVerbKindShift);
  }
  return classGrammars;
}

[[nodiscard]] std::vector<SourceRow>
collectReadings(ImageData& image, DescriptionIds& descriptionIds) {
  std::vector<std::array<bool, facts::kStemColumnCount>> orthSeen(
      image.dictionaries.size());
  std::vector<bool> meaningSeen(image.dictionaries.size());
  std::vector<SourceRow> source;
  source.reserve(expand::resultCount());

  std::uint32_t sourceOrder = 0;
  for (std::size_t index = 0; index < expand::resultCount(); ++index) {
    const expand::Result listed{expand::ResultIndex{index}};
    if (listed.fate() != expand::Fate::kept)
      continue;
    const std::string_view spelling = listed.spelling();
    const expand::Reading reading = listed.reading();
    const FormAddress address = addressOf(listed.origin());
    if (spelling.size() > rel::kMaximumWordLength)
      error::fatal("relationship emitter: a generated form exceeds the "
                   "image's 24-character spelling length");
    const Rendered result = render(reading);
    const std::uint32_t orth = image.strings.intern(result.orth);
    const std::uint32_t meaning = image.strings.intern(result.meaning);

    if (address.dictionary >= image.dictionaries.size() ||
        address.dictionary >= (1u << 16) || address.stemSlot < 1 ||
        address.stemSlot > facts::kStemColumnCount)
      error::fatal("relationship emitter: dictionary or stem mapping is out "
                   "of bounds");
    DictionaryRecord& dictionary = image.dictionaries[address.dictionary];
    const std::size_t slot = std::size_t{address.stemSlot} - 1;
    if (orthSeen[address.dictionary][slot] && dictionary.orth[slot] != orth)
      error::fatal("relationship emitter: one dictionary stem renders two "
                   "orthographies");
    dictionary.orth[slot] = orth;
    orthSeen[address.dictionary][slot] = true;
    if (meaningSeen[address.dictionary] && dictionary.meaning != meaning)
      error::fatal(
          "relationship emitter: one dictionary entry renders two meanings");
    dictionary.meaning = meaning;
    dictionary.age = std::to_underlying(reading.entryAge);
    dictionary.frequency = std::to_underlying(reading.entryFrequency);
    meaningSeen[address.dictionary] = true;

    const Analysis analysis{
        static_cast<std::uint16_t>(address.dictionary),
        static_cast<std::uint8_t>(slot),
        descriptionId(result.analysis, image, descriptionIds)};
    source.push_back(SourceRow{normalize(spelling), analysis, sourceOrder++});
  }
  return source;
}

// NOTE: Spellings in order for the automaton; within one spelling the rows
//  keep expansion order, which is the order a caller reads them back in.
void groupSpellings(std::vector<SourceRow> source, ImageData& image) {
  std::ranges::sort(source, [](const SourceRow& a, const SourceRow& b) {
    return a.word != b.word ? a.word < b.word : a.sourceOrder < b.sourceOrder;
  });
  image.words.reserve(source.size());
  image.analyses.reserve(source.size());
  for (std::size_t i = 0; i < source.size();) {
    const std::size_t begin = i;
    while (i < source.size() && source[i].word == source[begin].word)
      ++i;
    const std::size_t count = i - begin;
    if (count == 0 || count > rel::kMaximumResultsPerSpelling)
      error::fatal("relationship emitter: a spelling's result count exceeds "
                   "image capacity");
    image.words.push_back(Word{
        source[begin].word, static_cast<std::uint32_t>(image.analyses.size()),
        static_cast<std::uint16_t>(count)});
    for (std::size_t row = begin; row < i; ++row)
      image.analyses.push_back(source[row].analysis);
  }
}

// INFO: Search_Dictionaries reads DICTLINE stems directly, so a suffix can
//  reach a stem no inflection joined. One record per (spelling, key).
void listFallbackStems(ImageData& image) {
  for (std::uint32_t index = 0; index < expand::kDictlineRows; ++index) {
    const expand::DictlineEntry entry =
        expand::entry(expand::DictlineRow{index});
    std::set<std::pair<std::string, std::uint8_t>> seen;
    for (std::uint8_t column = 1; column <= facts::kStemColumnCount; ++column) {
      const word::Stem text = expand::stemAt(entry, latin::StemKey{column});
      if (word::stemState(text) != word::StemState::text)
        continue;
      const std::uint8_t key = expand::internal::stemKeyOfColumn(entry, column);
      if (key > 9)
        continue;
      if (!seen.emplace(normalize(text), key).second)
        continue;
      image.fallbackStems.push_back(FallbackStemRecord{
          image.strings.intern(normalize(text)),
          static_cast<std::uint16_t>(index), key, partOf(entry.grammar)});
    }
  }
  std::ranges::sort(
      image.fallbackStems, [&image](const FallbackStemRecord& left,
                                    const FallbackStemRecord& right) {
        const std::string_view a = image.strings.blob().c_str() + left.text;
        const std::string_view b = image.strings.blob().c_str() + right.text;
        return std::tie(a, left.dictionary, left.key) <
               std::tie(b, right.dictionary, right.key);
      });
}

// INFO: Run_Inflections scans INFLECTS.LAT rows only, never the synthetic ones.
[[nodiscard]] EndingOrder orderInflections(ImageData& image) {
  EndingOrder endingOrder;
  endingOrder.reserve(expand::kInflectsRows);
  for (std::uint32_t row = 0; row < expand::kInflectsRows; ++row)
    endingOrder.emplace_back(
        normalize(word::Inflects{word::InflectsRow{row}}.ending()), row);
  std::ranges::sort(endingOrder);

  image.inflections.reserve(endingOrder.size());
  for (const auto& [ending, row] : endingOrder) {
    const expand::InflectsEntry inflection =
        expand::inflection(expand::InflectsRow{row});
    image.inflections.push_back(InflectionRecord{
        .ending = image.strings.intern(ending),
        .key = inflection.stemKey.value,
        .allow = inflectionAllow(inflection),
        .age = std::to_underlying(inflection.age),
        .frequency = std::to_underlying(inflection.frequency)});
  }
  for (std::uint32_t row = 0; row < endingOrder.size();) {
    const std::uint32_t begin = row;
    while (row < endingOrder.size() &&
           endingOrder[row].first == endingOrder[begin].first)
      ++row;
    image.endings.push_back(EndingRecord{
        image.inflections[begin].ending, static_cast<std::uint16_t>(begin),
        static_cast<std::uint16_t>(row - begin)});
  }
  return endingOrder;
}

// INFO: The main join materialised for prefix readings, which start from a
//  stem, not a spelling. No stem-key gate: the runtime applies the entry's key.
void licenseClasses(const std::vector<latin::Entry>& classGrammars,
                    const EndingOrder& endingOrder, ImageData& image,
                    DescriptionIds& descriptionIds) {
  for (std::uint16_t id = 0; id < classGrammars.size(); ++id) {
    const latin::Entry& grammar = classGrammars[id];
    ClassRecord& record = image.classes[id];
    const bool packon =
        partOf(grammar) == latin::Part::PACK &&
        ((record.gate >> rel::kGatePackonShift) & rel::kGatePackonMask) != 0;
    record.rowStart = static_cast<std::uint32_t>(image.fallbackRows.size());
    for (std::uint32_t position = 0; position < endingOrder.size();
         ++position) {
      const expand::InflectsEntry inflection =
          expand::inflection(expand::InflectsRow{endingOrder[position].second});
      if (!(packon
                ? expand::internal::packonLicenses(grammar, inflection.grammar)
                : expand::internal::licenses(grammar, inflection.grammar)))
        continue;
      fallbackRow(grammar, inflection, position, image, descriptionIds);
    }
    record.rowCount =
        static_cast<std::uint16_t>(image.fallbackRows.size() - record.rowStart);
  }
}

// INFO: A suffix replaces the grammar outright, so (target, inflection) alone
//  names the parse line; the joined entry is gated separately on class and key.
void licenseSuffixes(const AddonBuckets& addonRows, std::size_t suffixBase,
                     const EndingOrder& endingOrder, ImageData& image,
                     DescriptionIds& descriptionIds) {
  for (std::size_t index = 0; index < addonRows.suffixes.size(); ++index) {
    const auto suffix = std::get<latin::addon::Suffix>(
        addonAt(addonRows.suffixes[index]).addon());
    const std::optional<latin::Entry> target = addonGrammar(suffix.grammar);
    AddonRecord& record = image.addons[suffixBase + index];
    if (!target)
      continue;
    const latin::Part targetPart = partOf(*target);
    record.targetGate = static_cast<std::uint8_t>(grammarGate(*target, 0));
    record.rowStart = static_cast<std::uint32_t>(image.fallbackRows.size());
    for (std::uint32_t position = 0; position < endingOrder.size();
         ++position) {
      const expand::InflectsEntry inflection =
          expand::inflection(expand::InflectsRow{endingOrder[position].second});
      if (!expand::internal::licenses(*target, inflection.grammar))
        continue;
      if (!latin::stemKeyAdmits(record.targetKey, inflection.stemKey.value,
                                targetPart))
        continue;
      fallbackRow(*target, inflection, position, image, descriptionIds);
    }
    record.rowCount =
        static_cast<std::uint16_t>(image.fallbackRows.size() - record.rowStart);
  }
}

} // namespace

[[nodiscard]] ImageData buildImageData() {
  ImageData image;
  const AddonBuckets addonRows = classifyAddons();
  const std::size_t suffixBase = internAddons(addonRows, image);
  const std::vector<latin::Entry> classGrammars =
      internClasses(packonOrdinals(addonRows), image);
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
