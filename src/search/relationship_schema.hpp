#pragma once

#include "facts.hpp"
#include "latin.hpp"

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
  States,
  Transitions,
  Lexemes,
  Paradigms,
  Targets,
  Instructions,
  Dictionaries,
  Addons,
  Classes,
  FallbackRows,
  Inflections,
  Endings,
  FallbackStems,
  Strings,
  Grammars,
  Entries,
  StemPatterns,
  count,
};

inline constexpr std::array<char, 8> kMagic{'W', 'H', 'I', 'T', 'R', 'E', 'L', '\0'};
inline constexpr std::uint32_t kVersion = 7;
inline constexpr std::uint32_t kSectionCount = static_cast<std::uint32_t>(Section::count);

// Record sizes. Every section but Strings is an array of one record.
inline constexpr std::size_t kTargetBytes = 2;
inline constexpr std::size_t kStemPatternBytes = 4;
inline constexpr std::size_t kGrammarBytes = 4;
inline constexpr std::size_t kEntryBytes = 6;

inline constexpr unsigned kStemLengthBits = 5;

inline constexpr std::uint16_t kRuleAbbreviation = 1u << 0;
inline constexpr std::uint16_t kRuleInterjOrConj = 1u << 1;
inline constexpr std::uint16_t kRuleConjThreeOne = 1u << 2;
inline constexpr std::uint16_t kRulePackonShift = 8;
inline constexpr std::uint16_t kRulePackonMask = 0xfu;
// The verb kind sits beside the class id, not in the class: two verbs of one conjugation share a class and can differ
// in kind.
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
  constexpr auto fields(this auto& self) noexcept { return std::tie(self.offset, self.bytes, self.count); }
};

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
  latin::NumeralValue numeralValue{};
  friend constexpr bool operator==(Entry, Entry) = default;
};

// Named exactly as the latin structs' members: latin_test.cpp matches them by name.
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

[[nodiscard]] constexpr unsigned widthOf(Field field) noexcept {
  switch (field) {
  case Field::numeralValue:
    return 10;
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

[[nodiscard]] constexpr bool fieldValid(Field field, std::uint16_t value) noexcept {
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
    return latin::isValid(latin::NumeralValue{value});
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

[[nodiscard]] constexpr Layout grammarLayout(latin::Part part) noexcept {
  switch (part) {
  case latin::Part::N:
  case latin::Part::PRON:
    return grammar_layout::kNominal;
  case latin::Part::ADJ:
    return grammar_layout::kAdjective;
  case latin::Part::NUM:
    return grammar_layout::kNumeral;
  case latin::Part::V:
    return grammar_layout::kVerb;
  case latin::Part::VPAR:
    return grammar_layout::kParticiple;
  case latin::Part::SUPINE:
    return grammar_layout::kSupine;
  case latin::Part::ADV:
    return grammar_layout::kAdverb;
  case latin::Part::PREP:
    return grammar_layout::kPreposition;
  case latin::Part::CONJ:
  case latin::Part::INTERJ:
    return std::span<const Field>{};
  case latin::Part::X:
  case latin::Part::PACK:
  case latin::Part::NONE:
    break;
  }
  return std::nullopt;
}

[[nodiscard]] constexpr Layout entryLayout(latin::Part part) noexcept {
  switch (part) {
  case latin::Part::N:
    return entry_layout::kNoun;
  case latin::Part::PRON:
    return entry_layout::kPronoun;
  case latin::Part::V:
    return entry_layout::kVerb;
  case latin::Part::ADJ:
    return entry_layout::kAdjective;
  case latin::Part::NUM:
    return entry_layout::kNumeral;
  case latin::Part::PACK:
    return entry_layout::kPackon;
  case latin::Part::ADV:
    return entry_layout::kAdverb;
  case latin::Part::PREP:
  case latin::Part::CONJ:
  case latin::Part::INTERJ:
    return std::span<const Field>{};
  case latin::Part::X:
  case latin::Part::SUPINE:
  case latin::Part::VPAR:
  case latin::Part::NONE:
    break;
  }
  return std::nullopt;
}

[[nodiscard]] constexpr std::uint16_t fieldOf(const latin::Analysis& a, Field field) noexcept {
  switch (field) {
  case Field::declension:
  case Field::conjugation:
    return a.which;
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
  default:
    return 0;
  }
}

constexpr void setField(latin::Analysis& a, Field field, std::uint16_t value) noexcept {
  const auto byte = static_cast<std::uint8_t>(value);
  switch (field) {
  case Field::declension:
  case Field::conjugation:
    a.which = byte;
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
  default:
    break;
  }
}

[[nodiscard]] constexpr std::uint16_t fieldOf(const Entry& e, Field field) noexcept {
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
  default:
    return 0;
  }
}

[[nodiscard]] constexpr latin::Part partOfWord(std::uint64_t word) noexcept {
  return latin::Part{static_cast<std::uint8_t>(word & ((std::uint64_t{1} << kPartBits) - 1))};
}

[[nodiscard]] constexpr std::uint64_t packGrammar(const latin::Analysis& a) noexcept {
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

[[nodiscard]] constexpr UnpackedGrammar unpackGrammar(std::uint64_t word) noexcept {
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

// Fixed slots, whatever the part.
struct EntrySlot {
  unsigned shift{};
  unsigned width{};
};

[[nodiscard]] constexpr EntrySlot slotOf(Field field) noexcept {
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
    return {24, 10};
  case Field::area:
    return {34, 4};
  case Field::geography:
    return {38, 5};
  case Field::source:
    return {43, 5};
  default:
    return {};
  }
}

[[nodiscard]] constexpr std::uint16_t slotValue(std::uint64_t word, Field field) noexcept {
  const EntrySlot slot = slotOf(field);
  return static_cast<std::uint16_t>((word >> slot.shift) & ((std::uint64_t{1} << slot.width) - 1));
}

[[nodiscard]] constexpr std::uint64_t packEntry(const Entry& e) noexcept {
  std::uint64_t word = std::to_underlying(e.part);
  for (const std::span<const Field> fields :
       {entryLayout(e.part).value_or(std::span<const Field>{}), std::span<const Field>{entry_layout::kLabels}})
    for (const Field field : fields)
      word |= std::uint64_t{fieldOf(e, field)} << slotOf(field).shift;
  return word;
}

// NOTE: Built as words, not field by field, on purpose.
[[nodiscard]] constexpr Entry decodeEntry(std::uint64_t word) noexcept {
  std::array<std::uint32_t, sizeof(Entry) / 4> words{};
  const auto put = [&words](std::size_t at, std::uint32_t value) { words[at / 4] |= value << (8 * (at % 4)); };
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

[[nodiscard]] constexpr unsigned bitsOf(std::span<const Field> fields) noexcept {
  unsigned bits = 0;
  for (const Field field : fields)
    bits += widthOf(field);
  return bits;
}

[[nodiscard]] constexpr bool layoutsFit() noexcept {
  for (unsigned p = 0; p <= std::to_underlying(latin::Part::NONE); ++p) {
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

[[nodiscard]] constexpr std::uint64_t readWord(const unsigned char* p, std::size_t bytes) noexcept {
  std::uint64_t word = 0;
  for (std::size_t i = 0; i < bytes; ++i)
    word |= std::uint64_t{p[i]} << (8 * i);
  return word;
}

[[nodiscard]] constexpr std::uint16_t read16(const unsigned char* p) noexcept {
  return static_cast<std::uint16_t>(p[0] | p[1] << 8);
}

[[nodiscard]] constexpr std::uint32_t read32(const unsigned char* p) noexcept {
  return std::uint32_t{p[0]} | std::uint32_t{p[1]} << 8 | std::uint32_t{p[2]} << 16 | std::uint32_t{p[3]} << 24;
}

[[nodiscard]] constexpr std::uint64_t read64(const unsigned char* p) noexcept {
  return read32(p) | (static_cast<std::uint64_t>(read32(p + 4)) << 32);
}

[[nodiscard]] constexpr std::uint32_t popcount32(std::uint32_t mask) noexcept {
  return static_cast<std::uint32_t>(std::popcount(mask));
}

struct Run {
  std::uint32_t first{};
  std::uint32_t count{};
  [[nodiscard]] constexpr std::uint32_t end() const noexcept { return first + count; }
  [[nodiscard]] constexpr bool empty() const noexcept { return count == 0; }
};

// Records as stored: `fields()` lists the members in stored order, each written little-endian at its own width. Gen
// writes from it, the library and the check read with it.
struct ClassRecord {
  std::uint32_t rowStart{};
  std::uint16_t rowCount{};
  std::uint16_t rules{};
  constexpr auto fields(this auto& self) noexcept { return std::tie(self.rowStart, self.rowCount, self.rules); }
  [[nodiscard]] constexpr Run rows() const noexcept { return {rowStart, rowCount}; }
};

struct FallbackRowRecord {
  std::uint16_t inflect{};
  std::uint16_t description{};
  constexpr auto fields(this auto& self) noexcept { return std::tie(self.inflect, self.description); }
};

struct InflectionRecord {
  std::uint32_t ending{};
  std::uint8_t stemKey{};
  std::uint8_t allow{};
  latin::Age age{};
  latin::Frequency frequency{};
  constexpr auto fields(this auto& self) noexcept {
    return std::tie(self.ending, self.stemKey, self.allow, self.age, self.frequency);
  }
};

struct HeaderRecord {
  std::array<char, 8> magic{};
  std::uint32_t version{};
  std::uint32_t headerBytes{};
  std::uint64_t imageBytes{};
  std::uint32_t sectionCount{};
  std::uint32_t root{};
  std::uint32_t rootOutput{};
  std::uint32_t maximumWordLength{};
  std::uint32_t spellingCount{};
  std::uint32_t resultCount{};
  constexpr auto fields(this auto& self) noexcept {
    return std::tie(self.magic, self.version, self.headerBytes, self.imageBytes, self.sectionCount, self.root,
                    self.rootOutput, self.maximumWordLength, self.spellingCount, self.resultCount);
  }
};

// The spelling automaton: a state's transitions are one per letter of its mask, in letter order, from firstTransition.
struct StateRecord {
  std::uint32_t firstTransition{};
  std::uint32_t letterMask{};
  std::uint8_t resultCount{};
  std::array<std::uint8_t, 3> reserved{};
  constexpr auto fields(this auto& self) noexcept {
    return std::tie(self.firstTransition, self.letterMask, self.resultCount, self.reserved);
  }
};

struct TransitionRecord {
  std::uint32_t target{};
  std::uint32_t output{};
  constexpr auto fields(this auto& self) noexcept { return std::tie(self.target, self.output); }
};

struct LexemeRecord {
  std::uint16_t dictionary{};
  std::uint8_t stem{};
  std::uint8_t reserved{};
  std::uint16_t paradigm{};
  std::uint16_t reservedEnd{};
  constexpr auto fields(this auto& self) noexcept {
    return std::tie(self.dictionary, self.stem, self.reserved, self.paradigm, self.reservedEnd);
  }
};

struct ParadigmRecord {
  std::uint32_t firstTarget{};
  std::uint8_t count{};
  std::array<std::uint8_t, 3> reserved{};
  constexpr auto fields(this auto& self) noexcept { return std::tie(self.firstTarget, self.count, self.reserved); }
};

struct DictionaryRecord {
  // Each column's printed stem: `pattern << 5 | length`.
  std::array<std::uint16_t, facts::kStemColumnCount> stems{};
  std::uint32_t meaning{};
  latin::Age age{};
  latin::Frequency frequency{};
  std::uint16_t classId{};
  constexpr auto fields(this auto& self) noexcept {
    return std::tie(self.stems, self.meaning, self.age, self.frequency, self.classId);
  }
};

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
  constexpr auto fields(this auto& self) noexcept {
    return std::tie(self.fix, self.meaning, self.target, self.kind, self.root, self.targetPart, self.rootStemKey,
                    self.targetStemKey, self.connectingLetter, self.targetRules, self.rowStart, self.rowCount,
                    self.firstLetterAsSpelled, self.reserved);
  }
  [[nodiscard]] constexpr Run rows() const noexcept { return {rowStart, rowCount}; }
};

struct EndingRecord {
  std::uint32_t text{};
  std::uint16_t first{};
  std::uint16_t count{};
  constexpr auto fields(this auto& self) noexcept { return std::tie(self.text, self.first, self.count); }
  [[nodiscard]] constexpr Run inflections() const noexcept { return {first, count}; }
};

struct FallbackStemRecord {
  std::uint32_t text{};
  std::uint16_t dictionary{};
  std::uint8_t stemKey{};
  latin::Part part{};
  constexpr auto fields(this auto& self) noexcept {
    return std::tie(self.text, self.dictionary, self.stemKey, self.part);
  }
};

template <typename T> struct IsArray : std::false_type {};
template <typename T, std::size_t N> struct IsArray<std::array<T, N>> : std::true_type {};

template <typename T> [[nodiscard]] constexpr T readField(const unsigned char* p) noexcept {
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
    return read16(p);
  else if constexpr (sizeof(T) == 4)
    return read32(p);
  else
    return read64(p);
}

template <typename Record> [[nodiscard]] constexpr Record readRecord(const unsigned char* p) noexcept {
  Record record{};
  std::apply(
      [&p](auto&... field) {
        ((field = readField<std::remove_reference_t<decltype(field)>>(p), p += sizeof field), ...);
      },
      record.fields());
  return record;
}

template <typename T> constexpr void storeField(unsigned char* p, const T& value) noexcept {
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

template <typename Record> constexpr void storeRecord(unsigned char* p, const Record& record) noexcept {
  std::apply([&p](const auto&... field) { ((storeField(p, field), p += sizeof field), ...); }, record.fields());
}

template <typename Record> [[nodiscard]] consteval std::size_t recordBytes() noexcept {
  Record record{};
  return std::apply([](auto&... field) { return (sizeof field + ...); }, record.fields());
}

inline constexpr std::size_t kHeaderBytes = recordBytes<HeaderRecord>();
inline constexpr std::size_t kStateBytes = recordBytes<StateRecord>();
inline constexpr std::size_t kTransitionBytes = recordBytes<TransitionRecord>();
inline constexpr std::size_t kLexemeBytes = recordBytes<LexemeRecord>();
inline constexpr std::size_t kParadigmBytes = recordBytes<ParadigmRecord>();
inline constexpr std::size_t kDirectoryBytes = recordBytes<DirectoryEntry>();
inline constexpr std::size_t kClassBytes = recordBytes<ClassRecord>();
inline constexpr std::size_t kFallbackRowBytes = recordBytes<FallbackRowRecord>();
inline constexpr std::size_t kInflectionBytes = recordBytes<InflectionRecord>();
inline constexpr std::size_t kFallbackStemBytes = recordBytes<FallbackStemRecord>();
inline constexpr std::size_t kDictionaryBytes = recordBytes<DictionaryRecord>();
inline constexpr std::size_t kAddonBytes = recordBytes<AddonRecord>();
inline constexpr std::size_t kEndingBytes = recordBytes<EndingRecord>();

// One byte for the instruction stream and the string pool: their directory counts are not records.
[[nodiscard]] constexpr std::size_t widthOf(Section section) noexcept {
  switch (section) {
  case Section::States:
    return kStateBytes;
  case Section::Transitions:
    return kTransitionBytes;
  case Section::Lexemes:
    return kLexemeBytes;
  case Section::Paradigms:
    return kParadigmBytes;
  case Section::Targets:
    return kTargetBytes;
  case Section::Instructions:
  case Section::Strings:
    return 1;
  case Section::Dictionaries:
    return kDictionaryBytes;
  case Section::Addons:
    return kAddonBytes;
  case Section::Classes:
    return kClassBytes;
  case Section::FallbackRows:
    return kFallbackRowBytes;
  case Section::Inflections:
    return kInflectionBytes;
  case Section::Endings:
    return kEndingBytes;
  case Section::FallbackStems:
    return kFallbackStemBytes;
  case Section::Grammars:
    return kGrammarBytes;
  case Section::Entries:
    return kEntryBytes;
  case Section::StemPatterns:
    return kStemPatternBytes;
  case Section::count:
    break;
  }
  return 0;
}

} // namespace whitaker::relationship
