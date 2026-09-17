#include "image_builder.hpp"

#include "error/error.hpp"
#include "facts.hpp"
#include "gen/emitter/wl_stable/detail/rendered_analysis.hpp"
#include "gen/expand/wl_stable/common/licensing.hpp"
#include "gen/expand/wl_stable/common/rows.hpp"
#include "gen/expand/wl_stable/common/stem_column.hpp"
#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "gen/expand/wl_stable/facts/forms.hpp"
#include "gen/expand/wl_stable/facts/readings.hpp"
#include "gen/expand/wl_stable/facts/swept.hpp"
#include "src/search/relationship_schema.hpp"
#include "string_section.hpp"
#include "types/tokenized_sources.hpp"
#include "types/types.hpp"

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
namespace render = wl_stable::emitter::detail::rendered_analysis;
namespace relationship_index = wl_stable::emitter::detail::relationship_index;

namespace wl_stable::emitter::detail::relationship_image {

namespace {

static_assert(facts::kMaxWordCharacters == rel::kMaximumWordLength,
              "the expansion's widest form and the relationship image's "
              "spelling capacity must agree");

[[nodiscard]] std::string normalize(std::string_view text) {
  std::string result{text};
  for (char& c : result)
    c = facts::foldLetter(c);
  return result;
}

struct SourceRow {
  std::string word;
  relationship_index::Analysis analysis;
  std::uint32_t sourceOrder{};
};

// INFO: ADDONS.LAT orders its parts differently from DICTLINE; both convert
//  here and neither source ordinal reaches the image.
static_assert(static_cast<int>(PosTokenTypes::N) ==
                      static_cast<int>(rel::Part::N) &&
                  static_cast<int>(PosTokenTypes::PRON) ==
                      static_cast<int>(rel::Part::PRON) &&
                  static_cast<int>(PosTokenTypes::V) ==
                      static_cast<int>(rel::Part::V) &&
                  static_cast<int>(PosTokenTypes::ADJ) ==
                      static_cast<int>(rel::Part::ADJ) &&
                  static_cast<int>(PosTokenTypes::ADV) ==
                      static_cast<int>(rel::Part::ADV) &&
                  static_cast<int>(PosTokenTypes::PREP) ==
                      static_cast<int>(rel::Part::PREP) &&
                  static_cast<int>(PosTokenTypes::NUM) ==
                      static_cast<int>(rel::Part::NUM) &&
                  static_cast<int>(PosTokenTypes::CONJ) ==
                      static_cast<int>(rel::Part::CONJ) &&
                  static_cast<int>(PosTokenTypes::INTERJ) ==
                      static_cast<int>(rel::Part::INTERJ) &&
                  static_cast<int>(PosTokenTypes::PACK) ==
                      static_cast<int>(rel::Part::PACK) &&
                  static_cast<int>(PosTokenTypes::SUPINE) ==
                      static_cast<int>(rel::Part::SUPINE) &&
                  static_cast<int>(PosTokenTypes::VPAR) ==
                      static_cast<int>(rel::Part::VPAR) &&
                  static_cast<int>(PosTokenTypes::NONE) ==
                      static_cast<int>(rel::Part::NONE),
              "the image's part ordinal must follow the generator's");

[[nodiscard]] std::uint8_t imagePart(PosTokenTypes part) noexcept {
  if (part == PosTokenTypes::X)
    return static_cast<std::uint8_t>(rel::Part::Any);
  return static_cast<std::uint8_t>(part);
}

[[nodiscard]] std::uint8_t
imageTargetPart(const tokenized::addon::Target& value) {
  return std::visit(
      [](const auto& target) -> std::uint8_t {
        using Target = std::remove_cvref_t<decltype(target)>;
        if constexpr (std::is_same_v<Target, tokenized::Noun>)
          return imagePart(PosTokenTypes::N);
        else if constexpr (std::is_same_v<Target, tokenized::Pronoun>)
          return imagePart(PosTokenTypes::PRON);
        else if constexpr (std::is_same_v<Target, tokenized::Packon>)
          return imagePart(PosTokenTypes::PACK);
        else if constexpr (std::is_same_v<Target, tokenized::Adjective>)
          return imagePart(PosTokenTypes::ADJ);
        else if constexpr (std::is_same_v<Target, tokenized::Numeral>)
          return imagePart(PosTokenTypes::NUM);
        else if constexpr (std::is_same_v<Target, tokenized::Adverb>)
          return imagePart(PosTokenTypes::ADV);
        else if constexpr (std::is_same_v<Target, tokenized::Verb>)
          return imagePart(PosTokenTypes::V);
        else
          return static_cast<std::uint8_t>(rel::Part::Any);
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

[[nodiscard]] AddonRecord
makePrefixRecord(const tokenized::addon::Prefix& prefix,
                 std::string_view meaning, rel::AddonKind kind,
                 StringSection& strings) {
  return AddonRecord{
      .fix = strings.intern(normalize(prefix.fix)),
      .meaning = strings.intern(meaning),
      .kind = kind,
      .root = imagePart(prefix.from),
      .targetPart = imagePart(prefix.to),
      .connect = static_cast<std::uint8_t>(prefix.connect),
      .firstRaw = static_cast<std::uint8_t>(
          prefix.fix.empty() ? '\0' : prefix.fix.front()),
  };
}

void writeAddonTarget(AddonRecord& result,
                      const tokenized::addon::Target& value) {
  result.targetPart = imageTargetPart(value);
  std::visit(
      [&result](const auto& target) {
        using Target = std::remove_cvref_t<decltype(target)>;
        if constexpr (std::is_same_v<Target, tokenized::Noun>) {
          result.target = {addonValue(target.declension),
                           addonValue(target.declensionVariant),
                           addonValue(target.gender),
                           addonValue(target.nounKind)};
        } else if constexpr (std::is_same_v<Target, tokenized::Pronoun>) {
          result.target = {addonValue(target.declension),
                           addonValue(target.declensionVariant),
                           addonValue(target.pronounKind), 0};
        } else if constexpr (std::is_same_v<Target, tokenized::Packon>) {
          result.target = {addonValue(target.declension),
                           addonValue(target.declensionVariant),
                           addonValue(target.packonKind), 0};
        } else if constexpr (std::is_same_v<Target, tokenized::Adjective>) {
          result.target = {addonValue(target.declension),
                           addonValue(target.declensionVariant),
                           addonValue(target.comparison), 0};
        } else if constexpr (std::is_same_v<Target, tokenized::Numeral>) {
          result.target = {addonValue(target.declension),
                           addonValue(target.declensionVariant),
                           addonValue(target.numeralSort),
                           addonValue(target.numeralValue)};
        } else if constexpr (std::is_same_v<Target, tokenized::Adverb>) {
          result.target = {addonValue(target.comparison), 0, 0, 0};
        } else if constexpr (std::is_same_v<Target, tokenized::Verb>) {
          result.target = {addonValue(target.conjugation),
                           addonValue(target.conjugationVariant),
                           addonValue(target.verbKind), 0};
        }
      },
      value);
}

[[nodiscard]] AddonRecord
makeSuffixRecord(const tokenized::addon::Suffix& addon,
                 std::string_view meaning, StringSection& strings) {
  AddonRecord result{
      .fix = strings.intern(normalize(addon.fix)),
      .meaning = strings.intern(meaning),
      .kind = rel::AddonKind::Suffix,
      .root = imagePart(addon.from),
      .rootKey = addon.fromKey.value,
      .targetKey = addon.toKey.value,
      .connect = static_cast<std::uint8_t>(addon.connect),
      .firstRaw = static_cast<std::uint8_t>(
          addon.fix.empty() ? '\0' : addon.fix.front()),
  };
  writeAddonTarget(result, addon.grammar);
  return result;
}

[[nodiscard]] AddonRecord
makeTackonRecord(const tokenized::addon::Tackon& tackon,
                 std::string_view meaning, rel::AddonKind kind,
                 StringSection& strings) {
  AddonRecord result{
      .fix = strings.intern(normalize(tackon.fix)),
      .meaning = strings.intern(meaning),
      .kind = kind,
      .root = static_cast<std::uint8_t>(rel::Part::Any),
      .firstRaw = static_cast<std::uint8_t>(
          tackon.fix.empty() ? '\0' : tackon.fix.front()),
  };
  writeAddonTarget(result, tackon.grammar);
  return result;
}

// NOTE: Any and Packon targets have no grammar to license against.
[[nodiscard]] std::optional<tokenized::Grammar>
addonGrammar(const tokenized::addon::Target& value) {
  return std::visit(
      [](const auto& target) -> std::optional<tokenized::Grammar> {
        using Target = std::remove_cvref_t<decltype(target)>;
        if constexpr (std::is_same_v<Target, tokenized::addon::Any> ||
                      std::is_same_v<Target, tokenized::Packon>)
          return std::nullopt;
        else
          return tokenized::Grammar{target};
      },
      value);
}

using GrammarKey = std::array<std::uint16_t, 4>;

// INFO: A class keys on the fields the join and parse line read; verb kind
//  travels on the entry, and a PACK entry's packon (from its gloss) keys.
[[nodiscard]] GrammarKey grammarKey(const tokenized::Grammar& grammar,
                                    std::uint16_t packon) {
  const PosTokenTypes part = render::partOf(grammar);
  GrammarKey key{static_cast<std::uint16_t>(part), 0, 0, packon};
  std::visit(
      [&key](const auto& value) {
        using Grammar = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::is_same_v<Grammar, tokenized::Noun>)
          key = {key[0], addonValue(value.declension),
                 addonValue(value.declensionVariant), addonValue(value.gender)};
        else if constexpr (std::is_same_v<Grammar, tokenized::Pronoun> ||
                           std::is_same_v<Grammar, tokenized::Packon>)
          key = {key[0], addonValue(value.declension),
                 addonValue(value.declensionVariant), key[3]};
        else if constexpr (std::is_same_v<Grammar, tokenized::Verb>)
          key = {key[0], addonValue(value.conjugation),
                 addonValue(value.conjugationVariant), 0};
        else if constexpr (std::is_same_v<Grammar, tokenized::Adjective>)
          key = {key[0], addonValue(value.declension),
                 addonValue(value.declensionVariant),
                 addonValue(value.comparison)};
        else if constexpr (std::is_same_v<Grammar, tokenized::Numeral>)
          key = {key[0], addonValue(value.declension),
                 addonValue(value.declensionVariant),
                 addonValue(value.numeralSort)};
        else if constexpr (std::is_same_v<Grammar, tokenized::Adverb>)
          key = {key[0], addonValue(value.comparison), 0, 0};
        else if constexpr (std::is_same_v<Grammar, tokenized::Preposition>)
          key = {key[0], addonValue(value.caseOf), 0, 0};
      },
      grammar);
  return key;
}

// INFO: word_package.adb:882-888; the asymmetry is Whitaker's.
[[nodiscard]] bool stemKeyAdmits(std::uint8_t dictKey, std::uint8_t wanted,
                                 PosTokenTypes part) noexcept {
  if (dictKey == wanted)
    return true;
  return dictKey == 0 && wanted >= 1 && wanted <= 2 &&
         (part == PosTokenTypes::N || part == PosTokenTypes::ADJ ||
          part == PosTokenTypes::V);
}

[[nodiscard]] int ageOrdinal(TypeAge age) noexcept {
  switch (age) {
  case TypeAge::X:
  case TypeAge::NONE:
    return 0;
  case TypeAge::A:
    return 1;
  case TypeAge::B:
    return 2;
  case TypeAge::C:
    return 3;
  case TypeAge::D:
    return 4;
  case TypeAge::E:
    return 5;
  case TypeAge::F:
    return 6;
  case TypeAge::G:
    return 7;
  case TypeAge::H:
    return 8;
  }
  return 0;
}

[[nodiscard]] int frequencyOrdinal(TypeFrequency frequency) noexcept {
  switch (frequency) {
  case TypeFrequency::X:
  case TypeFrequency::NONE:
    return 0;
  case TypeFrequency::A:
    return 1;
  case TypeFrequency::B:
    return 2;
  case TypeFrequency::C:
    return 3;
  case TypeFrequency::D:
    return 4;
  case TypeFrequency::E:
    return 5;
  case TypeFrequency::F:
    return 6;
  case TypeFrequency::I:
    return 7;
  case TypeFrequency::M:
    return 8;
  case TypeFrequency::N:
    return 9;
  }
  return 0;
}

[[nodiscard]] constexpr bool moodIndToInf(TypeMood mood) noexcept {
  return mood == TypeMood::IND || mood == TypeMood::SUB ||
         mood == TypeMood::IMP || mood == TypeMood::INF;
}

[[nodiscard]] constexpr bool moodIndToImp(TypeMood mood) noexcept {
  return mood == TypeMood::IND || mood == TypeMood::SUB ||
         mood == TypeMood::IMP;
}

[[nodiscard]] constexpr bool tensePresToFut(TypeTense tense) noexcept {
  return tense == TypeTense::PRES || tense == TypeTense::IMPF ||
         tense == TypeTense::FUT;
}

[[nodiscard]] constexpr bool tensePerfToFutp(TypeTense tense) noexcept {
  return tense == TypeTense::PERF || tense == TypeTense::PLUP ||
         tense == TypeTense::FUTP;
}

[[nodiscard]] std::string_view
packonOfGloss(std::string_view meaning) noexcept {
  constexpr std::string_view open{"(w/-"};
  if (!meaning.starts_with(open))
    return {};
  const std::size_t close = meaning.find(')', open.size());
  if (close == std::string_view::npos)
    return {};
  return meaning.substr(open.size(), close - open.size());
}

[[nodiscard]] std::uint8_t
stemKeyOfColumn(const wl_stable::expand::DictlineEntry& entry,
                std::uint8_t column) {
  bool servesOne = false;
  bool servesTwo = false;
  std::uint8_t lowest = 10;
  for (std::uint8_t key = 0; key <= 9; ++key) {
    const domain::StemIndex selected =
        wl_stable::expand::internal::columnForKey(entry,
                                                  domain::StemIndex{key});
    if (selected.value != column)
      continue;
    servesOne = servesOne || key == 1;
    servesTwo = servesTwo || key == 2;
    lowest = std::min(lowest, key);
  }
  return servesOne && servesTwo ? std::uint8_t{0} : lowest;
}

[[nodiscard]] bool
packonLicenses(const tokenized::Grammar& dictionary,
               const tokenized::Inflection& inflection) noexcept {
  const auto* packon = std::get_if<tokenized::Packon>(&dictionary);
  const auto* pronoun = std::get_if<tokenized::inflected::Pronoun>(&inflection);
  return packon != nullptr && pronoun != nullptr &&
         wl_stable::expand::internal::declensionMatches(
             packon->declension, packon->declensionVariant, pronoun->declension,
             pronoun->declensionVariant);
}

[[nodiscard]] std::uint8_t verbKindGate(TypeVerbKind kind) noexcept {
  switch (kind) {
  case TypeVerbKind::IMPERS:
    return rel::kVerbKindImpers;
  case TypeVerbKind::DEP:
    return rel::kVerbKindDep;
  case TypeVerbKind::SEMIDEP:
    return rel::kVerbKindSemidep;
  default:
    return rel::kVerbKindOther;
  }
}

[[nodiscard]] std::uint8_t
entryVerbKind(const tokenized::Grammar& grammar) noexcept {
  if (const auto* verb = std::get_if<tokenized::Verb>(&grammar))
    return verbKindGate(verb->verbKind);
  return rel::kVerbKindOther;
}

[[nodiscard]] std::uint16_t grammarGate(const tokenized::Grammar& grammar,
                                        std::uint16_t packon) noexcept {
  const PosTokenTypes part = render::partOf(grammar);
  std::uint16_t gate =
      static_cast<std::uint16_t>(packon << rel::kGatePackonShift);
  if (part == PosTokenTypes::CONJ || part == PosTokenTypes::INTERJ)
    gate |= rel::kGateInterjOrConj;
  std::visit(
      [&gate](const auto& value) {
        using Grammar = std::remove_cvref_t<decltype(value)>;
        if constexpr (requires { value.declension; }) {
          if (value.declension.value == 9 && value.declensionVariant.value == 8)
            gate |= rel::kGateAbbreviation;
        }
        if constexpr (std::is_same_v<Grammar, tokenized::Verb>) {
          if (value.conjugation.value == 3 &&
              value.conjugationVariant.value == 1)
            gate |= rel::kGateConjThreeOne;
        }
      },
      grammar);
  return gate;
}

[[nodiscard]] std::uint8_t
inflectionAllow(const wl_stable::expand::InflectsEntry& inflection) noexcept {
  if (render::partOf(inflection.grammar) != PosTokenTypes::V)
    return 0;
  const auto& verb = std::get<tokenized::inflected::Verb>(inflection.grammar);
  std::uint8_t allow = rel::kAllowIsVerb;
  if (verb.tense == TypeTense::PRES && verb.voice == TypeVoice::ACTIVE &&
      verb.mood == TypeMood::IMP && verb.person.value == 2 &&
      verb.number == TypeNumber::S && inflection.characterCount.value == 0)
    allow |= rel::kAllowShortImp;
  if (verb.mood == TypeMood::IMP &&
      !((verb.tense == TypeTense::PRES && verb.person.value == 2) ||
        (verb.tense == TypeTense::FUT &&
         (verb.person.value == 2 || verb.person.value == 3))))
    allow |= rel::kAllowImpNoPerson;
  if (verb.person.value != 3)
    allow |= rel::kAllowNotThird;
  if (verb.voice == TypeVoice::ACTIVE && verb.mood == TypeMood::INF &&
      verb.tense == TypeTense::FUT)
    allow |= rel::kAllowDepKeep;
  else if (verb.voice == TypeVoice::ACTIVE && moodIndToInf(verb.mood))
    allow |= rel::kAllowDepDrop;
  if (moodIndToImp(verb.mood) &&
      ((verb.voice == TypeVoice::PASSIVE && tensePresToFut(verb.tense)) ||
       (verb.voice == TypeVoice::ACTIVE && tensePerfToFutp(verb.tense))))
    allow |= rel::kAllowSemidepDrop;
  return allow;
}

struct AddonBuckets {
  std::vector<std::size_t> tickons;
  std::vector<std::size_t> prefixes;
  std::vector<std::size_t> suffixes;
  std::vector<std::size_t> tackons;
  std::vector<std::size_t> packons;
};

[[nodiscard]] bool isPackon(const tokenized::addon::Tackon& tackon,
                            std::string_view meaning) {
  const auto* grammar = std::get_if<tokenized::Packon>(&tackon.grammar);
  return grammar != nullptr &&
         (grammar->declension.value == 1 || grammar->declension.value == 2) &&
         meaning.starts_with("PACKON w/");
}

[[nodiscard]] AddonBuckets classifyAddons(const tokenized::Addons& addons) {
  AddonBuckets result;
  for (std::size_t row = 0; row < addons.addon.size(); ++row) {
    const tokenized::Addon& addon = addons.addon[row];
    if (const auto* prefix = std::get_if<tokenized::addon::Prefix>(&addon)) {
      (prefix->to == PosTokenTypes::PACK ? result.tickons : result.prefixes)
          .push_back(row);
    } else if (std::holds_alternative<tokenized::addon::Suffix>(addon)) {
      result.suffixes.push_back(row);
    } else {
      const auto& tackon = std::get<tokenized::addon::Tackon>(addon);
      (isPackon(tackon, addons.meaning[row]) ? result.packons : result.tackons)
          .push_back(row);
    }
  }
  return result;
}

struct FormAddress {
  std::uint32_t dictionary{};
  std::uint8_t stemSlot{};
};

[[nodiscard]] FormAddress addressOf(const wl_stable::expand::Origin& origin) {
  constexpr std::uint32_t uniqueBase =
      wl_stable::expand::kDictlineRows +
      wl_stable::expand::kSyntheticDictlineRows;
  return std::visit(
      []<typename T>(const T& value) -> FormAddress {
        if constexpr (std::is_same_v<T, wl_stable::expand::Joined>)
          return {value.entry.value, value.column.value};
        else
          return {uniqueBase + value.entry.value, 1};
      },
      origin);
}

using DescriptionIds =
    std::map<std::pair<std::string, std::string>, std::uint32_t>;
using EndingOrder = std::vector<std::pair<std::string, std::uint32_t>>;

[[nodiscard]] std::uint16_t describe(std::string_view pos,
                                     std::string_view line, ImageData& image,
                                     DescriptionIds& descriptionIds) {
  auto [description, inserted] = descriptionIds.emplace(
      std::pair{std::string{pos}, std::string{line}},
      static_cast<std::uint32_t>(image.descriptions.size()));
  if (inserted)
    image.descriptions.push_back(DescriptionRecord{image.strings.intern(pos),
                                                   image.strings.intern(line)});
  if (description->second > std::numeric_limits<std::uint16_t>::max())
    error::fatal("relationship emitter: description exceeds the target's "
                 "16-bit capacity");
  return static_cast<std::uint16_t>(description->second);
}

void fallbackRow(const tokenized::Grammar& grammar,
                 const wl_stable::expand::InflectsEntry& inflection,
                 std::uint32_t position, ImageData& image,
                 DescriptionIds& descriptionIds) {
  const std::string line =
      render::describeAnalysis(grammar, inflection.grammar, inflection.stemKey);
  const std::uint16_t description = describe(
      toName(render::partOf(inflection.grammar)), line, image, descriptionIds);
  image.fallbackRows.push_back(
      FallbackRowRecord{static_cast<std::uint16_t>(position), description});
}

// Returns the index of the first suffix record.
std::size_t internAddons(const tokenized::Addons& addons,
                         const AddonBuckets& addonRows, ImageData& image) {
  image.addons.reserve(addonRows.tickons.size() + addonRows.prefixes.size() +
                       addonRows.suffixes.size() + addonRows.tackons.size() +
                       addonRows.packons.size());
  for (const std::size_t row : addonRows.tickons)
    image.addons.push_back(makePrefixRecord(
        std::get<tokenized::addon::Prefix>(addons.addon[row]),
        addons.meaning[row], rel::AddonKind::Tickon, image.strings));
  for (const std::size_t row : addonRows.prefixes)
    image.addons.push_back(makePrefixRecord(
        std::get<tokenized::addon::Prefix>(addons.addon[row]),
        addons.meaning[row], rel::AddonKind::Prefix, image.strings));
  const std::size_t suffixBase = image.addons.size();
  for (const std::size_t row : addonRows.suffixes)
    image.addons.push_back(
        makeSuffixRecord(std::get<tokenized::addon::Suffix>(addons.addon[row]),
                         addons.meaning[row], image.strings));
  for (const std::size_t row : addonRows.tackons)
    image.addons.push_back(makeTackonRecord(
        std::get<tokenized::addon::Tackon>(addons.addon[row]),
        addons.meaning[row], rel::AddonKind::Tackon, image.strings));
  for (const std::size_t row : addonRows.packons)
    image.addons.push_back(makeTackonRecord(
        std::get<tokenized::addon::Tackon>(addons.addon[row]),
        addons.meaning[row], rel::AddonKind::Packon, image.strings));
  return suffixBase;
}

// NOTE: Ordinals start at 1 so that an entry naming no packon reads as 0.
[[nodiscard]] std::map<std::string, std::uint16_t>
packonOrdinals(const tokenized::Addons& addons, const AddonBuckets& addonRows) {
  std::map<std::string, std::uint16_t> result;
  for (std::size_t index = 0; index < addonRows.packons.size(); ++index) {
    const std::uint16_t ordinal = static_cast<std::uint16_t>(index + 1);
    if (ordinal > rel::kGatePackonMask)
      error::fatal("relationship emitter: more packons than the class gate "
                   "can name");
    const auto& packon = std::get<tokenized::addon::Tackon>(
        addons.addon[addonRows.packons[index]]);
    result.emplace(normalize(packon.fix), ordinal);
  }
  return result;
}

// INFO: Whitaker's fix rules read a class where the regular join reads an
//  entry. Returns the grammar each class id stands for.
[[nodiscard]] std::vector<tokenized::Grammar>
internClasses(const tokenized::Sources& sources,
              const wl_stable::expand::Synthetics& synthetics,
              const std::map<std::string, std::uint16_t>& packons,
              ImageData& image) {
  image.dictionaries.resize(wl_stable::expand::kDictlineRows +
                            wl_stable::expand::kSyntheticDictlineRows +
                            wl_stable::expand::kUniquesRows);
  std::map<GrammarKey, std::uint16_t> classIds;
  std::vector<tokenized::Grammar> classGrammars;
  for (std::uint32_t index = 0; index < wl_stable::expand::kDictlineRows;
       ++index) {
    const wl_stable::expand::DictlineEntry dictionary =
        wl_stable::expand::entry(sources, synthetics,
                                 wl_stable::expand::DictlineRow{index});
    std::uint16_t packon = 0;
    if (render::partOf(dictionary.grammar) == PosTokenTypes::PACK) {
      // NOTE: A packon ADDONS.LAT lacks leaves the class licensing nothing.
      const auto named =
          packons.find(normalize(packonOfGloss(dictionary.senses)));
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
      classGrammars.push_back(dictionary.grammar);
      image.classes.push_back(
          ClassRecord{.gate = grammarGate(dictionary.grammar, packon)});
    }
    image.dictionaries[index].age =
        static_cast<std::uint8_t>(ageOrdinal(dictionary.age));
    image.dictionaries[index].frequency =
        static_cast<std::uint8_t>(frequencyOrdinal(dictionary.frequency));
    image.dictionaries[index].classId =
        entry->second |
        static_cast<std::uint16_t>(entryVerbKind(dictionary.grammar)
                                   << rel::kClassVerbKindShift);
  }
  return classGrammars;
}

// Every kept reading, in expansion order, with its dictionary record filled.
[[nodiscard]] std::vector<SourceRow>
collectReadings(const wl_stable::expand::Forms& forms,
                const wl_stable::expand::Readings& readings,
                const wl_stable::expand::Swept& swept, ImageData& image,
                DescriptionIds& descriptionIds) {
  std::vector<std::array<bool, facts::kStemColumnCount>> orthSeen(
      image.dictionaries.size());
  std::vector<bool> meaningSeen(image.dictionaries.size());
  std::vector<SourceRow> source;
  std::size_t formCount = 0;
  for (const auto& byLetter : forms.byLetter)
    formCount += byLetter.size();
  source.reserve(formCount);

  std::uint32_t sourceOrder = 0;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const auto& listed = forms.byLetter[letter];
    const auto& read = readings.byLetter[letter];
    const auto& fates = swept.byLetter[letter];
    if (listed.size() != read.size() || listed.size() != fates.size())
      error::fatal("relationship emitter: expansion facts are not aligned");
    for (std::size_t index = 0; index < listed.size(); ++index) {
      if (fates[index] != wl_stable::expand::Fate::kept)
        continue;
      const wl_stable::expand::Form& form = listed[index];
      const wl_stable::expand::Reading& reading = read[index];
      const FormAddress address = addressOf(form.origin);
      if (form.spelling.size() > rel::kMaximumWordLength)
        error::fatal("relationship emitter: a generated form exceeds the "
                     "image's 24-character spelling length");
      const render::Rendered result = render::render(reading);
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
      dictionary.age = static_cast<std::uint8_t>(ageOrdinal(reading.entryAge));
      dictionary.frequency =
          static_cast<std::uint8_t>(frequencyOrdinal(reading.entryFrequency));
      meaningSeen[address.dictionary] = true;

      const relationship_index::Analysis analysis{
          static_cast<std::uint16_t>(address.dictionary),
          static_cast<std::uint8_t>(slot),
          describe(result.pos, result.inflection, image, descriptionIds)};
      source.push_back(
          SourceRow{normalize(form.spelling), analysis, sourceOrder++});
    }
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
    image.words.push_back(relationship_index::Word{
        source[begin].word, static_cast<std::uint32_t>(image.analyses.size()),
        static_cast<std::uint16_t>(count)});
    for (std::size_t row = begin; row < i; ++row)
      image.analyses.push_back(source[row].analysis);
  }
}

// INFO: Search_Dictionaries reads DICTLINE stems directly, so a suffix can
//  reach a stem no inflection joined. One record per (spelling, key).
void listFallbackStems(const tokenized::Sources& sources,
                       const wl_stable::expand::Synthetics& synthetics,
                       ImageData& image) {
  for (std::uint32_t index = 0; index < wl_stable::expand::kDictlineRows;
       ++index) {
    const wl_stable::expand::DictlineEntry entry = wl_stable::expand::entry(
        sources, synthetics, wl_stable::expand::DictlineRow{index});
    std::set<std::pair<std::string, std::uint8_t>> seen;
    for (std::uint8_t column = 1; column <= facts::kStemColumnCount; ++column) {
      const tokenized::Stem text =
          wl_stable::expand::stemAt(entry, domain::StemIndex{column});
      if (tokenized::stemState(text) != tokenized::StemState::text)
        continue;
      const std::uint8_t key = stemKeyOfColumn(entry, column);
      if (key > 9)
        continue;
      if (!seen.emplace(normalize(text), key).second)
        continue;
      image.fallbackStems.push_back(
          FallbackStemRecord{image.strings.intern(normalize(text)),
                             static_cast<std::uint16_t>(index), key,
                             imagePart(render::partOf(entry.grammar))});
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
// Returns the (ending, INFLECTS row) order the inflection table was written in.
[[nodiscard]] EndingOrder
orderInflections(const tokenized::Sources& sources,
                 const wl_stable::expand::Synthetics& synthetics,
                 ImageData& image) {
  EndingOrder endingOrder;
  endingOrder.reserve(wl_stable::expand::kInflectsRows);
  for (std::uint32_t row = 0; row < wl_stable::expand::kInflectsRows; ++row)
    endingOrder.emplace_back(normalize(sources.inflects.ending[row]), row);
  std::ranges::sort(endingOrder);

  image.inflections.reserve(endingOrder.size());
  for (const auto& [ending, row] : endingOrder) {
    const wl_stable::expand::InflectsEntry inflection =
        wl_stable::expand::inflection(sources, synthetics,
                                      wl_stable::expand::InflectsRow{row});
    image.inflections.push_back(InflectionRecord{
        .ending = image.strings.intern(ending),
        .key = inflection.stemKey.value,
        .allow = inflectionAllow(inflection),
        .age = static_cast<std::uint8_t>(ageOrdinal(inflection.age)),
        .frequency =
            static_cast<std::uint8_t>(frequencyOrdinal(inflection.frequency))});
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
void licenseClasses(const tokenized::Sources& sources,
                    const wl_stable::expand::Synthetics& synthetics,
                    const std::vector<tokenized::Grammar>& classGrammars,
                    const EndingOrder& endingOrder, ImageData& image,
                    DescriptionIds& descriptionIds) {
  for (std::uint16_t id = 0; id < classGrammars.size(); ++id) {
    const tokenized::Grammar& grammar = classGrammars[id];
    ClassRecord& record = image.classes[id];
    // NOTE: A PACK class licenses through the packon join once it has a packon.
    const bool packon =
        render::partOf(grammar) == PosTokenTypes::PACK &&
        ((record.gate >> rel::kGatePackonShift) & rel::kGatePackonMask) != 0;
    record.rowStart = static_cast<std::uint32_t>(image.fallbackRows.size());
    for (std::uint32_t position = 0; position < endingOrder.size();
         ++position) {
      const wl_stable::expand::InflectsEntry inflection =
          wl_stable::expand::inflection(
              sources, synthetics,
              wl_stable::expand::InflectsRow{endingOrder[position].second});
      if (!(packon ? packonLicenses(grammar, inflection.grammar)
                   : wl_stable::expand::internal::licenses(grammar,
                                                           inflection.grammar)))
        continue;
      fallbackRow(grammar, inflection, position, image, descriptionIds);
    }
    record.rowCount =
        static_cast<std::uint16_t>(image.fallbackRows.size() - record.rowStart);
  }
}

// INFO: A suffix replaces the grammar outright, so (target, inflection) alone
//  names the parse line; the joined entry is gated separately on class and key.
void licenseSuffixes(const tokenized::Sources& sources,
                     const wl_stable::expand::Synthetics& synthetics,
                     const AddonBuckets& addonRows, std::size_t suffixBase,
                     const EndingOrder& endingOrder, ImageData& image,
                     DescriptionIds& descriptionIds) {
  for (std::size_t index = 0; index < addonRows.suffixes.size(); ++index) {
    const auto& suffix = std::get<tokenized::addon::Suffix>(
        sources.addons.addon[addonRows.suffixes[index]]);
    const std::optional<tokenized::Grammar> target =
        addonGrammar(suffix.grammar);
    AddonRecord& record = image.addons[suffixBase + index];
    if (!target)
      continue;
    const PosTokenTypes targetPart = render::partOf(*target);
    record.targetGate = static_cast<std::uint8_t>(grammarGate(*target, 0));
    record.rowStart = static_cast<std::uint32_t>(image.fallbackRows.size());
    for (std::uint32_t position = 0; position < endingOrder.size();
         ++position) {
      const wl_stable::expand::InflectsEntry inflection =
          wl_stable::expand::inflection(
              sources, synthetics,
              wl_stable::expand::InflectsRow{endingOrder[position].second});
      if (!wl_stable::expand::internal::licenses(*target, inflection.grammar))
        continue;
      if (!stemKeyAdmits(record.targetKey, inflection.stemKey.value,
                         targetPart))
        continue;
      fallbackRow(*target, inflection, position, image, descriptionIds);
    }
    record.rowCount =
        static_cast<std::uint16_t>(image.fallbackRows.size() - record.rowStart);
  }
}

} // namespace

[[nodiscard]] ImageData
buildImageData(const tokenized::Sources& sources,
               const wl_stable::expand::Synthetics& synthetics,
               const wl_stable::expand::Forms& forms,
               const wl_stable::expand::Readings& readings,
               const wl_stable::expand::Swept& swept) {
  ImageData image;
  const AddonBuckets addonRows = classifyAddons(sources.addons);
  const std::size_t suffixBase = internAddons(sources.addons, addonRows, image);
  const std::vector<tokenized::Grammar> classGrammars = internClasses(
      sources, synthetics, packonOrdinals(sources.addons, addonRows), image);
  DescriptionIds descriptionIds;
  groupSpellings(collectReadings(forms, readings, swept, image, descriptionIds),
                 image);
  listFallbackStems(sources, synthetics, image);
  const EndingOrder endingOrder = orderInflections(sources, synthetics, image);
  licenseClasses(sources, synthetics, classGrammars, endingOrder, image,
                 descriptionIds);
  licenseSuffixes(sources, synthetics, addonRows, suffixBase, endingOrder,
                  image, descriptionIds);
  image.program = relationship_index::build(image.words, image.analyses);
  return image;
}

} // namespace wl_stable::emitter::detail::relationship_image
