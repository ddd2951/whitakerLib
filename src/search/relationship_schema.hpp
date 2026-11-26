#pragma once
#include <cstring>
#include <type_traits>

#include "facts.hpp"
#include "latin.hpp"
#include "two_ended_bounds.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

namespace whitaker::relationship {

enum class Section : std::uint32_t {
  Dictionaries,
  Addons,
  Classes,
  ClassRows,
  Inflections,
  Endings,
  Stems,
  Strings,
  Grammars,
  Entries,
  StemPatterns,
  InflectionLabels,
  TrimmedReadings,
  Front,
  Back,
  Rules,
  Cells,
  CellIndex,
  CellOf,
  RowIds,
  AffixBack,
  PrefixFront,
  SuffixBack,
  AddonIds,
  FrontRuns,
  DictionaryStemRuns,
  DictionaryStems,
  StemLetters,
  count,
};

inline constexpr std::array<char, 8> kMagic{'W', 'H', 'I', 'T', 'R', 'E', 'L', '\0'};
inline constexpr std::uint32_t kVersion = 12;
inline constexpr std::uint32_t kSectionCount = static_cast<std::uint32_t>(Section::count);

// INFO: Record sizes. Every section but Strings is an array of one record.
inline constexpr std::size_t kTargetBytes = 2;
inline constexpr std::size_t kStemPatternBytes = 4;
inline constexpr std::size_t kGrammarBytes = 4;
// INFO: Source entries use ten numeral bits; Roman readings never enter the image.
inline constexpr std::size_t kEntryBytes = 6;

inline constexpr unsigned kStemLengthBits = 5;
inline constexpr std::uint16_t kMissingStemCode = 1u << kStemLengthBits;

inline constexpr std::uint16_t kRuleAbbreviation = 1u << 0;
inline constexpr std::uint16_t kRuleInterjOrConj = 1u << 1;
inline constexpr std::uint16_t kRuleConjThreeOne = 1u << 2;
inline constexpr std::uint16_t kRulePackonShift = 8;
inline constexpr std::uint16_t kRulePackonMask = 0xfu;
// NOTE: The verb kind sits beside the class id, not in the class: two verbs of one conjugation share a class and can
//  differ in kind.
inline constexpr std::uint16_t kClassIdMask = 0x0fffu;
inline constexpr std::uint16_t kClassVerbKindShift = 12;
inline constexpr std::uint16_t kClassVerbKindMask = 0xfu;

inline constexpr std::uint8_t kLexemeChange = 0x80u;
inline constexpr std::uint8_t kRelationshipMask = 0x7fu;
inline constexpr std::size_t kMaximumResultsPerSpelling = 127;
inline constexpr std::size_t kMaximumTargetsPerParadigm = kRelationshipMask;

struct DirectoryEntry {
  std::uint64_t offset{};
  std::uint64_t bytes{};
  std::uint64_t count{};
  constexpr auto fields(this auto& self) { return std::tie(self.offset, self.bytes, self.count); }
};
static_assert(std::is_trivially_copyable_v<DirectoryEntry>);

struct Entry {
  latin::Part part{};
  std::uint8_t which{};
  std::uint8_t variant{};
  latin::Gender gender{};
  std::uint8_t kind{};
  latin::Comparison comparison{};
  latin::NumeralSort numeralSort{};
  latin::Area area{};
  latin::Geography geography{};
  latin::Source source{};
  latin::EntryNumeralValue numeralValue{};
  friend constexpr bool operator==(Entry, Entry) = default;
};
static_assert(std::is_trivially_copyable_v<Entry>);

// INFO: Named exactly as the latin structs' members: latin_test.cpp matches them by name.
enum class Field : unsigned char {
  declension,
  conjugation,
  declensionVariant,
  conjugationVariant,
  caseOf,
  number,
  gender,
  comparison,
  numeralSort,
  tense,
  voice,
  mood,
  person,
  nounKind,
  pronounKind,
  verbKind,
  packonKind,
  numeralValue,
  area,
  geography,
  source,
};

[[nodiscard]] constexpr unsigned widthOf(const Field field) {
  switch (field) {
    case Field::numeralValue:
      return static_cast<unsigned>(std::bit_width(latin::kEntryNumeralValueMaximum));
    case Field::geography:
    case Field::source:
      return 5;
    case Field::declension:
    case Field::conjugation:
    case Field::declensionVariant:
    case Field::conjugationVariant:
    case Field::nounKind:
    case Field::verbKind:
    case Field::area:
      return 4;
    case Field::caseOf:
    case Field::gender:
    case Field::numeralSort:
    case Field::tense:
    case Field::mood:
    case Field::pronounKind:
    case Field::packonKind:
      return 3;
    case Field::number:
    case Field::comparison:
    case Field::voice:
    case Field::person:
      return 2;
  }
  return 0;
}

[[nodiscard]] constexpr bool fieldValid(const Field field, const std::uint16_t value) {
  const auto byte = static_cast<std::uint8_t>(value);
  switch (field) {
    case Field::declension:
      return latin::isValid(latin::Declension{byte});
    case Field::conjugation:
      return latin::isValid(latin::Conjugation{byte});
    case Field::declensionVariant:
    case Field::conjugationVariant:
      return latin::isValid(latin::Variant{byte});
    case Field::person:
      return latin::isValid(latin::Person{byte});
    case Field::numeralValue:
      return latin::isValid(latin::EntryNumeralValue{value});
    case Field::caseOf:
      return latin::name(latin::Case{byte}) != nullptr;
    case Field::number:
      return latin::name(latin::Number{byte}) != nullptr;
    case Field::gender:
      return latin::name(latin::Gender{byte}) != nullptr;
    case Field::comparison:
      return latin::name(latin::Comparison{byte}) != nullptr;
    case Field::numeralSort:
      return latin::name(latin::NumeralSort{byte}) != nullptr;
    case Field::tense:
      return latin::name(latin::Tense{byte}) != nullptr;
    case Field::voice:
      return latin::name(latin::Voice{byte}) != nullptr;
    case Field::mood:
      return latin::name(latin::Mood{byte}) != nullptr;
    case Field::nounKind:
      return latin::name(latin::NounKind{byte}) != nullptr;
    case Field::pronounKind:
      return latin::name(latin::PronounKind{byte}) != nullptr;
    case Field::verbKind:
      return latin::name(latin::VerbKind{byte}) != nullptr;
    case Field::packonKind:
      return latin::name(latin::PackonKind{byte}) != nullptr;
    case Field::area:
      return latin::name(latin::Area{byte}) != nullptr;
    case Field::geography:
      return latin::name(latin::Geography{byte}) != nullptr;
    case Field::source:
      return latin::name(latin::Source{byte}) != nullptr;
  }
  return false;
}

inline constexpr unsigned kPartBits = 4;

namespace grammar_layout {
using enum Field;
inline constexpr Field kNominal[]{declension, declensionVariant, caseOf, number, gender};
inline constexpr Field kAdjective[]{declension, declensionVariant, caseOf, number, gender, comparison};
inline constexpr Field kNumeral[]{declension, declensionVariant, caseOf, number, gender, numeralSort};
inline constexpr Field kVerb[]{conjugation, conjugationVariant, tense, voice, mood, person, number};
inline constexpr Field kParticiple[]{conjugation, conjugationVariant, caseOf, number, gender, tense, voice, mood};
inline constexpr Field kSupine[]{conjugation, conjugationVariant, caseOf, number, gender};
inline constexpr Field kAdverb[]{comparison};
inline constexpr Field kPreposition[]{caseOf};
} // namespace grammar_layout

namespace entry_layout {
using enum Field;
inline constexpr Field kNoun[]{declension, declensionVariant, gender, nounKind};
inline constexpr Field kPronoun[]{declension, declensionVariant, pronounKind};
inline constexpr Field kVerb[]{conjugation, conjugationVariant, verbKind};
inline constexpr Field kAdjective[]{declension, declensionVariant, comparison};
inline constexpr Field kNumeral[]{declension, declensionVariant, numeralSort, numeralValue};
inline constexpr Field kPackon[]{declension, declensionVariant, packonKind};
inline constexpr Field kAdverb[]{comparison};
inline constexpr Field kLabels[]{area, geography, source};
} // namespace entry_layout

using Layout = std::optional<std::span<const Field>>;

[[nodiscard]] constexpr Layout grammarLayout(const latin::Part part) {
  switch (part) {
    case latin::Part::noun:
    case latin::Part::pronoun:
      return grammar_layout::kNominal;
    case latin::Part::adjective:
      return grammar_layout::kAdjective;
    case latin::Part::numeral:
      return grammar_layout::kNumeral;
    case latin::Part::verb:
      return grammar_layout::kVerb;
    case latin::Part::participle:
      return grammar_layout::kParticiple;
    case latin::Part::supine:
      return grammar_layout::kSupine;
    case latin::Part::adverb:
      return grammar_layout::kAdverb;
    case latin::Part::preposition:
      return grammar_layout::kPreposition;
    case latin::Part::conjunction:
    case latin::Part::interjection:
      return std::span<const Field>{};
    case latin::Part::any:
    case latin::Part::packon:
    case latin::Part::none:
      break;
  }
  return std::nullopt;
}

[[nodiscard]] constexpr Layout entryLayout(const latin::Part part) {
  switch (part) {
    case latin::Part::noun:
      return entry_layout::kNoun;
    case latin::Part::pronoun:
      return entry_layout::kPronoun;
    case latin::Part::verb:
      return entry_layout::kVerb;
    case latin::Part::adjective:
      return entry_layout::kAdjective;
    case latin::Part::numeral:
      return entry_layout::kNumeral;
    case latin::Part::packon:
      return entry_layout::kPackon;
    case latin::Part::adverb:
      return entry_layout::kAdverb;
    case latin::Part::preposition:
    case latin::Part::conjunction:
    case latin::Part::interjection:
      return std::span<const Field>{};
    case latin::Part::any:
    case latin::Part::supine:
    case latin::Part::participle:
    case latin::Part::none:
      break;
  }
  return std::nullopt;
}

[[nodiscard]] constexpr std::uint16_t fieldOf(const latin::Analysis& a, const Field field) {
  switch (field) {
    case Field::declension:
    case Field::conjugation:
      return a.which.value;
    case Field::declensionVariant:
    case Field::conjugationVariant:
      return a.variant.value;
    case Field::caseOf:
      return std::to_underlying(a.caseOf);
    case Field::number:
      return std::to_underlying(a.number);
    case Field::gender:
      return std::to_underlying(a.gender);
    case Field::comparison:
      return std::to_underlying(a.comparison);
    case Field::numeralSort:
      return std::to_underlying(a.numeralSort);
    case Field::tense:
      return std::to_underlying(a.tense);
    case Field::voice:
      return std::to_underlying(a.voice);
    case Field::mood:
      return std::to_underlying(a.mood);
    case Field::person:
      return a.person.value;
    case Field::nounKind:
    case Field::pronounKind:
    case Field::verbKind:
    case Field::packonKind:
    case Field::numeralValue:
    case Field::area:
    case Field::geography:
    case Field::source:
      break;
  }
  return 0;
}

constexpr void setField(latin::Analysis& a, const Field field, const std::uint16_t value) {
  const auto byte = static_cast<std::uint8_t>(value);
  switch (field) {
    case Field::declension:
    case Field::conjugation:
      a.which = {byte};
      break;
    case Field::declensionVariant:
    case Field::conjugationVariant:
      a.variant = {byte};
      break;
    case Field::caseOf:
      a.caseOf = latin::Case{byte};
      break;
    case Field::number:
      a.number = latin::Number{byte};
      break;
    case Field::gender:
      a.gender = latin::Gender{byte};
      break;
    case Field::comparison:
      a.comparison = latin::Comparison{byte};
      break;
    case Field::numeralSort:
      a.numeralSort = latin::NumeralSort{byte};
      break;
    case Field::tense:
      a.tense = latin::Tense{byte};
      break;
    case Field::voice:
      a.voice = latin::Voice{byte};
      break;
    case Field::mood:
      a.mood = latin::Mood{byte};
      break;
    case Field::person:
      a.person = {byte};
      break;
    case Field::nounKind:
    case Field::pronounKind:
    case Field::verbKind:
    case Field::packonKind:
    case Field::numeralValue:
    case Field::area:
    case Field::geography:
    case Field::source:
      break;
  }
}

[[nodiscard]] constexpr std::uint16_t fieldOf(const Entry& e, const Field field) {
  switch (field) {
    case Field::declension:
    case Field::conjugation:
      return e.which;
    case Field::declensionVariant:
    case Field::conjugationVariant:
      return e.variant;
    case Field::gender:
      return std::to_underlying(e.gender);
    case Field::nounKind:
    case Field::pronounKind:
    case Field::verbKind:
    case Field::packonKind:
      return e.kind;
    case Field::comparison:
      return std::to_underlying(e.comparison);
    case Field::numeralSort:
      return std::to_underlying(e.numeralSort);
    case Field::numeralValue:
      return e.numeralValue.value;
    case Field::area:
      return std::to_underlying(e.area);
    case Field::geography:
      return std::to_underlying(e.geography);
    case Field::source:
      return std::to_underlying(e.source);
    case Field::caseOf:
    case Field::number:
    case Field::tense:
    case Field::voice:
    case Field::mood:
    case Field::person:
      break;
  }
  return 0;
}

[[nodiscard]] constexpr latin::Part partOfWord(const std::uint64_t word) {
  return latin::Part{static_cast<std::uint8_t>(word & ((std::uint64_t{1} << kPartBits) - 1))};
}

[[nodiscard]] constexpr std::uint64_t packGrammar(const latin::Analysis& a) {
  std::uint64_t word = std::to_underlying(a.part);
  unsigned shift = kPartBits;
  for (const Field field : grammarLayout(a.part).value_or(std::span<const Field>{})) {
    word |= std::uint64_t{fieldOf(a, field)} << shift;
    shift += widthOf(field);
  }
  return word;
}

struct UnpackedGrammar {
  latin::Analysis analysis{};
  bool valid{};
};
static_assert(std::is_trivially_copyable_v<UnpackedGrammar>);

[[nodiscard]] constexpr UnpackedGrammar unpackGrammar(const std::uint64_t word) {
  UnpackedGrammar out{.analysis = {.part = partOfWord(word)}};
  const Layout layout = grammarLayout(out.analysis.part);
  out.valid = layout.has_value();
  unsigned shift = kPartBits;
  for (const Field field : layout.value_or(std::span<const Field>{})) {
    const auto value = static_cast<std::uint16_t>((word >> shift) & ((std::uint64_t{1} << widthOf(field)) - 1));
    out.valid = out.valid && fieldValid(field, value);
    setField(out.analysis, field, value);
    shift += widthOf(field);
  }
  out.valid = out.valid && (word >> shift) == 0;
  return out;
}

// INFO: Fixed slots, whatever the part.
struct EntrySlot {
  unsigned shift{};
  unsigned width{};
};
static_assert(std::is_trivially_copyable_v<EntrySlot>);

[[nodiscard]] constexpr EntrySlot slotOf(const Field field) {
  switch (field) {
    case Field::declension:
    case Field::conjugation:
      return {4, 4};
    case Field::declensionVariant:
    case Field::conjugationVariant:
      return {8, 4};
    case Field::gender:
      return {12, 3};
    case Field::nounKind:
    case Field::pronounKind:
    case Field::verbKind:
    case Field::packonKind:
      return {15, 4};
    case Field::comparison:
      return {19, 2};
    case Field::numeralSort:
      return {21, 3};
    case Field::numeralValue:
      return {24, widthOf(Field::numeralValue)};
    case Field::area:
      return {34, 4};
    case Field::geography:
      return {38, 5};
    case Field::source:
      return {43, 5};
    case Field::caseOf:
    case Field::number:
    case Field::tense:
    case Field::voice:
    case Field::mood:
    case Field::person:
      break;
  }
  return {};
}

[[nodiscard]] constexpr std::uint16_t slotValue(const std::uint64_t word, const Field field) {
  const EntrySlot slot = slotOf(field);
  return static_cast<std::uint16_t>((word >> slot.shift) & ((std::uint64_t{1} << slot.width) - 1));
}

[[nodiscard]] constexpr std::uint64_t packEntry(const Entry& e) {
  std::uint64_t word = std::to_underlying(e.part);
  for (const std::span<const Field> fields :
       {entryLayout(e.part).value_or(std::span<const Field>{}), std::span<const Field>{entry_layout::kLabels}})
    for (const Field field : fields)
      word |= std::uint64_t{fieldOf(e, field)} << slotOf(field).shift;
  return word;
}

// NOTE: Built as words, not field by field, on purpose.
[[nodiscard]] constexpr Entry decodeEntry(const std::uint64_t word) {
  std::array<std::uint32_t, sizeof(Entry) / 4> words{};
  const auto put = [&words](const std::size_t at, const std::uint32_t value) {
    words[at / 4] |= value << (8 * (at % 4));
  };
  put(offsetof(Entry, part), std::to_underlying(partOfWord(word)));
  put(offsetof(Entry, which), slotValue(word, Field::declension));
  put(offsetof(Entry, variant), slotValue(word, Field::declensionVariant));
  put(offsetof(Entry, gender), slotValue(word, Field::gender));
  put(offsetof(Entry, kind), slotValue(word, Field::nounKind));
  put(offsetof(Entry, comparison), slotValue(word, Field::comparison));
  put(offsetof(Entry, numeralSort), slotValue(word, Field::numeralSort));
  put(offsetof(Entry, area), slotValue(word, Field::area));
  put(offsetof(Entry, geography), slotValue(word, Field::geography));
  put(offsetof(Entry, source), slotValue(word, Field::source));
  put(offsetof(Entry, numeralValue), slotValue(word, Field::numeralValue));
  return std::bit_cast<Entry>(words);
}
static_assert(std::endian::native == std::endian::little, "decodeEntry lays Entry out little-endian");

[[nodiscard]] constexpr unsigned bitsOf(const std::span<const Field> fields) {
  unsigned bits = 0;
  for (const Field field : fields)
    bits += widthOf(field);
  return bits;
}

[[nodiscard]] constexpr bool layoutsFit() {
  for (unsigned p = 0; p <= std::to_underlying(latin::Part::none); ++p) {
    const latin::Part part{static_cast<std::uint8_t>(p)};
    const Layout grammar = grammarLayout(part);
    const Layout entry = entryLayout(part);
    if (grammar && kPartBits + bitsOf(*grammar) > 8 * kGrammarBytes)
      return false;
    if (entry)
      for (const std::span<const Field> fields : {*entry, std::span<const Field>{entry_layout::kLabels}})
        for (const Field field : fields)
          if (widthOf(field) > slotOf(field).width || slotOf(field).shift < kPartBits ||
              slotOf(field).shift + slotOf(field).width > 8 * kEntryBytes)
            return false;
  }
  return true;
}
static_assert(layoutsFit(), "a layout no longer fits its record");

[[nodiscard]] constexpr std::uint64_t readWord(const unsigned char* const p, const std::size_t bytes) {
  std::uint64_t word = 0;
  if constexpr (std::endian::native == std::endian::little)
    if (!std::is_constant_evaluated() && bytes == 8) {
      std::memcpy(&word, p, 8);
      return word;
    }
  for (std::size_t i = 0; i < bytes; ++i)
    word |= std::uint64_t{p[i]} << (8 * i);
  return word;
}

[[nodiscard]] constexpr std::uint16_t read16(const unsigned char* const p) {
  if constexpr (std::endian::native == std::endian::little)
    if (!std::is_constant_evaluated()) {
      std::uint16_t value;
      std::memcpy(&value, p, 2);
      return value;
    }
  return static_cast<std::uint16_t>(p[0] | p[1] << 8);
}

[[nodiscard]] constexpr std::uint32_t read32(const unsigned char* const p) {
  if constexpr (std::endian::native == std::endian::little)
    if (!std::is_constant_evaluated()) {
      std::uint32_t value;
      std::memcpy(&value, p, 4);
      return value;
    }
  return std::uint32_t{p[0]} | std::uint32_t{p[1]} << 8 | std::uint32_t{p[2]} << 16 | std::uint32_t{p[3]} << 24;
}

[[nodiscard]] constexpr std::uint64_t read64(const unsigned char* const p) {
  return read32(p) | (static_cast<std::uint64_t>(read32(p + 4)) << 32);
}

// INFO: A front trie slot. States are shared, so a slot holds no run: a walk counts the stems before it instead.
inline constexpr unsigned kFrontMaskBits = 26, kFrontFirstBits = 17, kFrontOffsetBits = 16;
[[nodiscard]] constexpr std::uint64_t frontSlot(const std::uint32_t mask, const std::uint32_t first,
                                                const std::uint32_t offset, const bool stem) {
  return std::uint64_t{mask} | std::uint64_t{first} << kFrontMaskBits |
         std::uint64_t{offset} << (kFrontMaskBits + kFrontFirstBits) |
         std::uint64_t{stem} << (kFrontMaskBits + kFrontFirstBits + kFrontOffsetBits);
}

struct Run {
  std::uint32_t first{};
  std::uint32_t count{};
};
static_assert(std::is_trivially_copyable_v<Run>);

[[nodiscard]] constexpr std::uint32_t end(const Run run) { return run.first + run.count; }
[[nodiscard]] constexpr bool empty(const Run run) { return run.count == 0; }

// INFO: Records as stored: `fields()` lists the members in stored order, each written little-endian at its own width.
//  Gen writes from it, the library and the check read with it.
struct ClassRecord {
  std::uint32_t rowStart{};
  std::uint16_t rowCount{};
  std::uint16_t rules{};
  constexpr auto fields(this auto& self) { return std::tie(self.rowStart, self.rowCount, self.rules); }
};
static_assert(std::is_trivially_copyable_v<ClassRecord>);

[[nodiscard]] constexpr Run rows(const ClassRecord record) { return {record.rowStart, record.rowCount}; }

struct ClassRowRecord {
  std::uint16_t inflect{};
  std::uint16_t description{};
  constexpr auto fields(this auto& self) { return std::tie(self.inflect, self.description); }
};
static_assert(std::is_trivially_copyable_v<ClassRowRecord>);

struct InflectionRecord {
  std::uint32_t ending{};
  std::uint8_t stemKey{};
  std::uint8_t allow{};
  latin::Age age{};
  latin::Frequency frequency{};
  constexpr auto fields(this auto& self) {
    return std::tie(self.ending, self.stemKey, self.allow, self.age, self.frequency);
  }
};
static_assert(std::is_trivially_copyable_v<InflectionRecord>);

struct HeaderRecord {
  std::array<char, 8> magic{};
  std::uint32_t version{};
  std::uint32_t headerBytes{};
  std::uint64_t imageBytes{};
  std::uint32_t sectionCount{};
  std::uint32_t maximumWordLength{};
  std::uint32_t spellingCount{};
  std::uint32_t resultCount{};
  constexpr auto fields(this auto& self) {
    return std::tie(self.magic, self.version, self.headerBytes, self.imageBytes, self.sectionCount,
                    self.maximumWordLength, self.spellingCount, self.resultCount);
  }
};
static_assert(std::is_trivially_copyable_v<HeaderRecord>);

struct DictionaryRecord {
  // INFO: Each column's printed stem: `pattern << 5 | length`, with kMissingStemCode for DICTLINE's zzz.
  std::array<std::uint16_t, facts::kStemColumnCount> stems{};
  std::uint32_t meaning{};
  latin::Age age{};
  latin::Frequency frequency{};
  std::uint16_t classId{};
  constexpr auto fields(this auto& self) {
    return std::tie(self.stems, self.meaning, self.age, self.frequency, self.classId);
  }
};
static_assert(std::is_trivially_copyable_v<DictionaryRecord>);

struct AddonRecord {
  std::uint32_t fix{};
  std::uint32_t meaning{};
  std::array<std::uint16_t, 4> target{};
  latin::AddonKind kind{};
  latin::Part root{};
  latin::Part targetPart{};
  std::uint8_t rootStemKey{};
  std::uint8_t targetStemKey{};
  std::uint8_t connectingLetter{};
  std::uint8_t targetRules{};
  std::uint32_t rowStart{};
  std::uint16_t rowCount{};
  std::uint8_t firstLetterAsSpelled{};
  std::uint16_t reserved{};
  constexpr auto fields(this auto& self) {
    return std::tie(self.fix, self.meaning, self.target, self.kind, self.root, self.targetPart, self.rootStemKey,
                    self.targetStemKey, self.connectingLetter, self.targetRules, self.rowStart, self.rowCount,
                    self.firstLetterAsSpelled, self.reserved);
  }
};
static_assert(std::is_trivially_copyable_v<AddonRecord>);

[[nodiscard]] constexpr Run rows(const AddonRecord& record) { return {record.rowStart, record.rowCount}; }

struct EndingRecord {
  std::uint32_t text{};
  std::uint16_t first{};
  std::uint16_t count{};
  constexpr auto fields(this auto& self) { return std::tie(self.text, self.first, self.count); }
};
static_assert(std::is_trivially_copyable_v<EndingRecord>);

[[nodiscard]] constexpr Run inflections(const EndingRecord record) { return {record.first, record.count}; }

// INFO: One per grammar and ending a reading has: the inflection's age and frequency. The ending is in the string pool,
//  shared with Inflections; records are sorted by description, then ending.
struct InflectionLabelRecord {
  std::uint32_t ending{};
  std::uint16_t description{};
  latin::Age age{};
  latin::Frequency frequency{};
  constexpr auto fields(this auto& self) { return std::tie(self.ending, self.description, self.age, self.frequency); }
};
static_assert(std::is_trivially_copyable_v<InflectionLabelRecord>);

// INFO: A PRON or ADJ reading List_Sweep trimmed from its spelling alone, kept for Try_Tackons, which sweeps only the
//  readings a tackon accepts (isdem: iisdem, trimmed from is beside the common is). Sorted by spelling.
inline constexpr std::uint8_t kTrimmedNotAllowed = 1;
inline constexpr std::uint8_t kTrimmedRare = 2;
struct TrimmedReadingRecord {
  std::uint32_t text{};
  std::uint16_t dictionary{};
  std::uint8_t stem{};
  std::uint8_t kind{};
  std::uint16_t description{};
  constexpr auto fields(this auto& self) {
    return std::tie(self.text, self.dictionary, self.stem, self.kind, self.description);
  }
};
static_assert(std::is_trivially_copyable_v<TrimmedReadingRecord>);

// INFO: Shared by the plain pass and affixes. 255 marks a stem used only by the plain pass.
inline constexpr std::uint16_t kNoLookupClass = 65535;
static_assert(kPartBits == 4 && facts::kStemColumnCount == 4);
struct StemRecord {
  std::uint16_t dictionary{};
  std::uint8_t stemKey = 255, partColumnFlags{};
  std::uint16_t klass = kNoLookupClass;
  constexpr auto fields(this auto& self) {
    return std::tie(self.dictionary, self.stemKey, self.partColumnFlags, self.klass);
  }
};
static_assert(std::is_trivially_copyable_v<StemRecord>);

[[nodiscard]] constexpr latin::Part part(const StemRecord record) { return latin::Part(record.partColumnFlags & 15); }
[[nodiscard]] constexpr std::uint8_t column(const StemRecord record) { return (record.partColumnFlags >> 4) & 3; }
[[nodiscard]] constexpr std::uint8_t flags(const StemRecord record) { return record.partColumnFlags >> 6; }

// INFO: Decoded node; generated widths pack its fields in the image.
struct Node {
  std::uint32_t mask{}, first{};
  Run run{};
};
static_assert(std::is_trivially_copyable_v<Node>);
struct Grammar {
  std::uint8_t rex{};
  std::array<std::uint8_t, 3> g{};
  constexpr auto fields(this auto& s) { return std::tie(s.rex, s.g); }
};
static_assert(std::is_trivially_copyable_v<Grammar>);
static_assert(kGrammarBytes <= 4, "Rule::quality holds the source grammar");
struct Rule {
  Grammar reading{};
  std::uint32_t order{}, arrayOrder{}, quality{};
  std::uint16_t source{}, ending{};
  std::uint8_t key{}, endingLength{};
  latin::Age age{};
  latin::Frequency frequency{};
  std::int16_t correction{};
  std::uint16_t description{};
  // INFO: What inflectionLabels returns for this rule's description and its own spelled suffix (age in the low four
  //  bits, frequency in the high four), and that suffix's length, so a reading spelled with it needs no search.
  std::uint8_t label{}, suffixLength{};
  constexpr auto fields(this auto& s) {
    return std::tie(s.reading.rex, s.reading.g, s.order, s.arrayOrder, s.quality, s.source, s.ending, s.key,
                    s.endingLength, s.age, s.frequency, s.correction, s.description, s.label, s.suffixLength);
  }
};
static_assert(std::is_trivially_copyable_v<Rule>);
// INFO: Labels and the printed stem come from DictionaryRecord, rather than another copy here.
struct Stem {
  std::uint16_t dictionary{}, klass{};
  std::uint8_t column{}, flags{};
  constexpr auto fields(this auto& s) { return std::tie(s.dictionary, s.klass, s.column, s.flags); }
};
static_assert(std::is_trivially_copyable_v<Stem>);
inline constexpr unsigned kUnique = 1, kQu = 2;

struct CellIndex {
  std::uint32_t mask{};
  std::uint16_t first{};
  constexpr auto fields(this auto& s) { return std::tie(s.mask, s.first); }
};
static_assert(std::is_trivially_copyable_v<CellIndex>);
struct Cell {
  std::uint16_t first{};
  std::uint8_t count{};
  KeepMask keep{};
  constexpr auto fields(this auto& s) { return std::tie(s.first, s.count, s.keep); }
};
static_assert(std::is_trivially_copyable_v<Cell>);

template <typename T> struct IsArray : std::false_type {};
template <typename T, std::size_t N> struct IsArray<std::array<T, N>> : std::true_type {};

template <typename T> [[nodiscard, gnu::always_inline]] constexpr T readField(const unsigned char* p) {
  if constexpr (IsArray<T>::value) {
    T items{};
    for (auto& item : items) {
      item = readField<typename T::value_type>(p);
      p += sizeof item;
    }
    return items;
  } else if constexpr (std::is_enum_v<T>)
    return T{readField<std::underlying_type_t<T>>(p)};
  else if constexpr (sizeof(T) == 1)
    return static_cast<T>(p[0]);
  else if constexpr (sizeof(T) == 2)
    return static_cast<T>(read16(p));
  else if constexpr (sizeof(T) == 4)
    return static_cast<T>(read32(p));
  else
    return static_cast<T>(read64(p));
}

template <typename Record> [[nodiscard, gnu::always_inline]] constexpr Record readRecord(const unsigned char* p) {
  Record record{};
  // INFO: Unpacked here rather than through std::apply, whose helper LTO may keep out of line: then every read builds
  //  the whole record field by field.
  auto fields = record.fields();
  [&]<std::size_t... I> [[gnu::always_inline]] (std::index_sequence<I...>) {
    ((std::get<I>(fields) = readField<std::remove_reference_t<std::tuple_element_t<I, decltype(fields)>>>(p),
      p += sizeof(std::get<I>(fields))),
     ...);
  }(std::make_index_sequence<std::tuple_size_v<decltype(fields)>>{});
  return record;
}

template <typename T> constexpr void storeField(unsigned char* p, const T& value) {
  if constexpr (IsArray<T>::value) {
    for (const auto& item : value) {
      storeField(p, item);
      p += sizeof item;
    }
  } else if constexpr (std::is_enum_v<T>) {
    storeField(p, std::to_underlying(value));
  } else {
    for (std::size_t i = 0; i < sizeof value; ++i)
      p[i] = static_cast<unsigned char>(static_cast<std::uint64_t>(value) >> (8 * i));
  }
}

template <typename Record> constexpr void storeRecord(unsigned char* p, const Record& record) {
  std::apply([&p](const auto&... field) { ((storeField(p, field), p += sizeof field), ...); }, record.fields());
}

template <typename Record> [[nodiscard]] consteval std::size_t recordBytes() {
  Record record{};
  return std::apply([](auto&... field) { return (sizeof field + ...); }, record.fields());
}

inline constexpr std::size_t kHeaderBytes = recordBytes<HeaderRecord>();
inline constexpr std::size_t kDirectoryBytes = recordBytes<DirectoryEntry>();
inline constexpr std::size_t kClassBytes = recordBytes<ClassRecord>();
inline constexpr std::size_t kClassRowBytes = recordBytes<ClassRowRecord>();
inline constexpr std::size_t kInflectionBytes = recordBytes<InflectionRecord>();
inline constexpr std::size_t kStemBytes = recordBytes<StemRecord>();
inline constexpr std::size_t kDictionaryBytes = recordBytes<DictionaryRecord>();
inline constexpr std::size_t kAddonBytes = recordBytes<AddonRecord>();
inline constexpr std::size_t kEndingBytes = recordBytes<EndingRecord>();
inline constexpr std::size_t kInflectionLabelBytes = recordBytes<InflectionLabelRecord>();
inline constexpr std::size_t kTrimmedReadingBytes = recordBytes<TrimmedReadingRecord>();

// INFO: One byte for the instruction stream and the string pool: their directory counts are not records.
[[nodiscard]] constexpr std::size_t widthOf(const Section section) {
  switch (section) {
    case Section::Strings:
      return 1;
    case Section::Dictionaries:
      return kDictionaryBytes;
    case Section::Addons:
      return kAddonBytes;
    case Section::Classes:
      return kClassBytes;
    case Section::ClassRows:
      return kClassRowBytes;
    case Section::Inflections:
      return kInflectionBytes;
    case Section::Endings:
      return kEndingBytes;
    case Section::Stems:
      return kStemBytes;
    case Section::Grammars:
      return kGrammarBytes;
    case Section::Entries:
      return kEntryBytes;
    case Section::StemPatterns:
      return kStemPatternBytes;
    case Section::InflectionLabels:
      return kInflectionLabelBytes;
    case Section::TrimmedReadings:
      return kTrimmedReadingBytes;
    case Section::Front:
    case Section::Back:
    case Section::AffixBack:
    case Section::PrefixFront:
    case Section::SuffixBack:
      return kNodeBytes;
    case Section::Rules:
      return recordBytes<Rule>();
    case Section::Cells:
      return recordBytes<Cell>();
    case Section::CellIndex:
      return recordBytes<CellIndex>();
    case Section::FrontRuns:
    case Section::DictionaryStemRuns:
    case Section::DictionaryStems:
    case Section::StemLetters:
    case Section::CellOf:
    case Section::RowIds:
    case Section::AddonIds:
      return 2;
    case Section::count:
      break;
  }
  return 0;
}

} // namespace whitaker::relationship
