#include "relationship_image.hpp"

#include <whitaker.hpp>

#include "dictionary_form.hpp"
#include "facts.hpp"
#include "image.hpp"
#include "two_ended.hpp"
#include "per_thread.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>
#include <type_traits>

namespace whitaker::relationship {
namespace {

[[nodiscard]] const unsigned char* at(const Section section, const std::uint64_t index) {
  return imageRecord(section, index);
}

[[nodiscard]] std::uint16_t addonCount() { return static_cast<std::uint16_t>(section(Section::Addons).count); }

[[nodiscard]] const char* stringAt(const std::uint32_t offset) { return imageStrings() + offset; }

template <typename Record>
[[nodiscard, gnu::always_inline]] inline Record read(const Section section, const std::uint64_t index) {
  return readRecord<Record>(at(section, index));
}

} // namespace

namespace {
static_assert(two_ended::kCandidateCapacity * 2 <= 255, "candidates and adverb corrections fit Program::count");
struct LookupScratch {
  two_ended::Lookup lookup{};
  std::vector<latin::packed::Reading> unused{};
};
[[nodiscard, gnu::always_inline]] inline LookupScratch& scratch() { return PerThread<LookupScratch>::get(); }
} // namespace

Program lookup(const std::string_view word) {
  const auto count = scratch().lookup.lookup<two_ended::Stage::compress>(word, scratch().unused);
  return {0, static_cast<std::uint8_t>(count), scratch().lookup.trimmed};
}

Result next(std::uint32_t& cursor, std::uint16_t& lexeme) {
  latin::packed::Reading r;
  std::uint16_t description;
  nextReading(cursor, lexeme, r, description);
  const auto code = dictionary(r.entry).stems[r.stem];
  return {static_cast<std::uint8_t>(code & ((1u << kStemLengthBits) - 1)),
          stringAt(read32(at(Section::StemPatterns, code >> kStemLengthBits))),
          grammar(description),
          description,
          r.entry,
          r.stem,
          r.stemKey};
}

void grammarReading(const std::uint16_t description, latin::packed::Reading& out) {
  if (!packedGrammars().empty()) {
    const auto& row = packedGrammars()[description];
    out = latin::packed::Reading{latin::packed::headChange(row.rex),
                                 latin::packed::headPart(row.rex),
                                 0,
                                 0,
                                 0,
                                 {row.g[0], row.g[1], row.g[2]},
                                 0};
  } else {
    const auto g = grammar(description);
    out = {};
    out.part = g.part;
    const bool fits = latin::packed::packGrammar(g, out.g);
    assert(fits);
    (void)fits;
  }
}

void nextReading(std::uint32_t& cursor, std::uint16_t& lexeme, latin::packed::Reading& out,
                 std::uint16_t& description) {
  (void)lexeme;
  const auto a = scratch().lookup.candidates[cursor++];
  const auto rule = two_ended::kRules[a.rule];
  const auto stem = two_ended::kStems[a.stem];
  description = rule.description;
  // INFO: A rule's key is its INFLECTS row's stem key; a unique's stem has none.
  const std::uint8_t key = (stem.flags & kUnique) != 0 ? 0 : rule.key;
  out = latin::packed::Reading{latin::packed::headChange(rule.reading.rex),
                               latin::packed::headPart(rule.reading.rex),
                               static_cast<std::uint8_t>(key & 7),
                               stem.column,
                               stem.dictionary,
                               {rule.reading.g[0], rule.reading.g[1], rule.reading.g[2]},
                               0};
}

TrimmedReadingRecord trimmedReading(const std::uint32_t index) {
  return read<TrimmedReadingRecord>(Section::TrimmedReadings, index);
}

Run trimmedReadingRange(const std::string_view folded) {
  const auto all =
      std::views::iota(std::uint32_t{0}, static_cast<std::uint32_t>(section(Section::TrimmedReadings).count));
  const auto found = std::ranges::equal_range(
      all, folded, {}, [](const std::uint32_t i) { return std::string_view{stringAt(trimmedReading(i).text)}; });
  return {static_cast<std::uint32_t>(found.begin() - all.begin()), static_cast<std::uint32_t>(found.size())};
}

Result resultOf(const TrimmedReadingRecord& record) {
  const std::uint16_t printed = dictionary(record.dictionary).stems[record.stem];
  return {static_cast<std::uint8_t>(printed & ((1u << kStemLengthBits) - 1)),
          stringAt(read32(at(Section::StemPatterns, printed >> kStemLengthBits))),
          grammar(record.description),
          record.description,
          record.dictionary,
          record.stem};
}

Entry entry(const std::uint16_t dictionary) {
  return decodeEntry(readWord(at(Section::Entries, dictionary), kEntryBytes));
}

DictionaryRecord dictionary(const std::uint16_t index) { return read<DictionaryRecord>(Section::Dictionaries, index); }

const char* dictionaryMeaning(const std::uint16_t index) { return stringAt(dictionary(index).meaning); }

ClassRecord dictionaryClass(const std::uint16_t index) { return read<ClassRecord>(Section::Classes, index); }

latin::Analysis grammar(const std::uint16_t description) {
  if (!packedGrammars().empty()) {
    const auto& p = packedGrammars()[description];
    return latin::packed::grammarOf(latin::packed::Reading{
        latin::packed::headChange(p.rex), latin::packed::headPart(p.rex), 0, 0, 0, {p.g[0], p.g[1], p.g[2]}, 0});
  }
  return sImage->grammars.empty() ? decodeGrammar(*sImage, description) : sImage->grammars[description];
}

ClassRowRecord classRow(const std::uint32_t index) { return read<ClassRowRecord>(Section::ClassRows, index); }

InflectionRecord inflection(const std::uint16_t index) { return read<InflectionRecord>(Section::Inflections, index); }

StemRecord stemRecord(const std::uint32_t index) { return read<StemRecord>(Section::Stems, index); }

static_assert(kMaxWordLength == facts::kMaxWordCharacters);
namespace {
void eachDictionaryStem(const std::uint16_t dictionary, const auto& visit) {
  if (dictionary + 1u >= section(Section::DictionaryStemRuns).count)
    return;
  const auto runs = std::views::iota(std::uint32_t{0}, static_cast<std::uint32_t>(section(Section::FrontRuns).count));
  const auto first = read16(at(Section::DictionaryStemRuns, dictionary));
  const auto end = read16(at(Section::DictionaryStemRuns, dictionary + 1u));
  for (std::uint32_t member = first; member < end; ++member) {
    const std::uint32_t row = read16(at(Section::DictionaryStems, member));
    const auto record = stemRecord(row);
    assert(relationship::dictionary(record.dictionary).stems[column(record)] != kMissingStemCode);
    // INFO: A stem row's spelling is the last whose run starts at or before it.
    const auto spelling = *std::ranges::prev(std::ranges::upper_bound(
        runs, row, {}, [](const std::uint32_t i) { return std::uint32_t{read16(at(Section::FrontRuns, i))}; }));
    whitaker::Stem stem{};
    const std::span<char, facts::kMaxWordCharacters> letters{stem.spelling, facts::kMaxWordCharacters};
    two_ended::walkFront(spelling, letters);
    two_ended::unfold(letters, read16(at(Section::StemLetters, row)));
    visit(record, stem);
  }
}
} // namespace

DictionaryWordStems stems(const std::uint16_t dictionary) {
  DictionaryWordStems out{};
  const auto push = [&out](const std::uint8_t key, const whitaker::Stem& stem) {
    assert(out.count < std::size(out.stems));
    auto at = out.count++;
    for (; at > 0 && out.stems[at - 1].key > key; --at)
      out.stems[at] = out.stems[at - 1];
    out.stems[at] = stem;
    out.stems[at].key = key;
  };
  eachDictionaryStem(dictionary, [&](const StemRecord record, const whitaker::Stem& stem) {
    // INFO: Key 0 is the first and second stems spelled alike. 255 is a row the affix tables never key: a unique's,
    //  which has no key, or esse's empty second stem, whose key is its column.
    if (record.stemKey == 0) {
      push(1, stem);
      push(2, stem);
    } else if (record.stemKey != 255)
      push(record.stemKey, stem);
    else
      push(dictionary >= std::to_underlying(kFirstUnique) ? 0 : static_cast<std::uint8_t>(column(record) + 1), stem);
  });
  return out;
}

DictionaryStemColumns dictionaryStemColumns(const std::uint16_t id) {
  const auto word = dictionary(id);
  DictionaryStemColumns columns{};
  eachDictionaryStem(id, [&](const StemRecord record, const whitaker::Stem& stem) {
    std::ranges::copy(stem.spelling, columns[column(record)].begin());
    // INFO: The two equal stems share one row in the first column.
    if (record.stemKey == 0)
      std::ranges::copy(stem.spelling, columns[1].begin());
  });
  for (std::size_t i = 0; i < columns.size(); ++i)
    if (word.stems[i] == kMissingStemCode)
      std::ranges::copy(kMissingStem, columns[i].begin());
  return columns;
}

Run stemRange(const std::string_view text) {
  // INFO: The front trie spells every stem row, so a row needs no text of its own.
  Run run{};
  two_ended::walkFront(text, [&](const unsigned cut, const std::uint32_t index) {
    if (cut == text.size())
      run = two_ended::frontRun(index);
  });
  return run;
}

StemEntry stemEntry(const std::uint32_t index) {
  const StemRecord stem = stemRecord(index);
  if (stem.stemKey == 255)
    return {.stemKey = 255};
  const DictionaryRecord entry = dictionary(stem.dictionary);
  const ClassRecord grammar = dictionaryClass(entry.classId & kClassIdMask);
  return StemEntry{.dictionary = stem.dictionary,
                   .stemKey = stem.stemKey,
                   .column = column(stem),
                   .part = part(stem),
                   .verbKind = latin::VerbKind{static_cast<std::uint8_t>((entry.classId >> kClassVerbKindShift) &
                                                                         kClassVerbKindMask)},
                   .rules = grammar.rules,
                   .age = entry.age,
                   .frequency = entry.frequency,
                   .rows = rows(grammar)};
}

Run endingRun(const std::string_view text) {
  const auto endings = std::views::iota(std::uint32_t{0}, static_cast<std::uint32_t>(section(Section::Endings).count));
  const auto ending = [](const std::uint32_t i) { return read<EndingRecord>(Section::Endings, i); };
  const auto found = std::ranges::lower_bound(
      endings, text, {}, [&](const std::uint32_t i) { return std::string_view{stringAt(ending(i).text)}; });
  if (found == endings.end() || std::string_view{stringAt(ending(*found).text)} != text)
    return {};
  return inflections(ending(*found));
}

std::pair<latin::Age, latin::Frequency> readingLabels(const std::uint32_t cursor, const std::uint16_t description,
                                                      const std::string_view spelling, const std::uint8_t length) {
  if (!sImage->packedGrammars.empty()) {
    const auto& row = sImage->packedGrammars[description];
    if (!row.labelsVary)
      return {row.age, row.frequency};
  }
  // INFO: Gen stores what inflectionLabels finds for the rule's description and spelled suffix; it applies when the
  //  spelling ends in that suffix after the stem.
  const auto rule = two_ended::kRules[scratch().lookup.candidates[cursor - 1].rule];
  if (length <= spelling.size() && spelling.size() - length == rule.suffixLength)
    return {latin::Age{static_cast<std::uint8_t>(rule.label & 15u)},
            latin::Frequency{static_cast<std::uint8_t>(rule.label >> 4)}};
  return readingLabels(description, spelling, length);
}

std::pair<latin::Age, latin::Frequency> readingLabels(const std::uint16_t description, const std::string_view spelling,
                                                      const std::uint8_t length) {
  if (!packedGrammars().empty()) {
    const auto& row = packedGrammars()[description];
    if (!row.labelsVary)
      return {row.age, row.frequency};
  }
  const auto ending = spelling.substr(length);
  char folded[facts::kMaxWordCharacters];
  std::ranges::transform(ending, folded, facts::foldLetter);
  return inflectionLabels(description, {folded, ending.size()});
}

std::pair<latin::Age, latin::Frequency> inflectionLabels(const std::uint16_t description,
                                                         const std::string_view ending) {
  constexpr std::size_t cacheSize = 4096;
  constexpr std::uint32_t descriptionMultiplier = 2654435761u;
  constexpr std::uint32_t endingMultiplier = 16777619u;

  struct Cached {
    const Image* image = nullptr;
    std::uint16_t description = 0;
    std::uint32_t ending = 0;
    latin::Age age{};
    latin::Frequency frequency{};
  };
  static_assert(std::is_trivially_copyable_v<Cached>);
  auto& cache = PerThread<std::array<Cached, cacheSize>>::get();

  Cached* saved = nullptr;
  // NOTE: Only embedded images have predecoded grammars; loaded images reuse one Image object.
  if (!packedGrammars().empty() || !sImage->grammars.empty()) {
    std::uint32_t hash = description * descriptionMultiplier;
    for (const char c : ending)
      hash = (hash ^ static_cast<unsigned char>(c)) * endingMultiplier;
    saved = &cache[hash & (cacheSize - 1)];
    if (saved->image == sImage && saved->description == description &&
        std::string_view{stringAt(saved->ending)} == ending)
      return {saved->age, saved->frequency};
  }

  const auto labels =
      std::views::iota(std::uint32_t{0}, static_cast<std::uint32_t>(section(Section::InflectionLabels).count));
  const auto label = [](const std::uint32_t i) { return read<InflectionLabelRecord>(Section::InflectionLabels, i); };
  const auto key = [&](const std::uint32_t i) {
    const InflectionLabelRecord r = label(i);
    return std::pair<std::uint16_t, std::string_view>{r.description, stringAt(r.ending)};
  };
  const std::pair<std::uint16_t, std::string_view> wanted{description, ending};
  const auto found = std::ranges::lower_bound(labels, wanted, {}, key);
  if (found == labels.end() || key(*found) != wanted)
    return {};
  const InflectionLabelRecord record = label(*found);
  if (saved != nullptr)
    *saved = {sImage, description, record.ending, record.age, record.frequency};
  return {record.age, record.frequency};
}

Addon addon(const std::uint16_t id) { return id < addonCount() ? decodeAddon(*sImage, id) : Addon{}; }

std::span<const Addon> addonsOf(const latin::AddonKind kind) { return sImage->addonsByKind[std::to_underlying(kind)]; }

const HeaderRecord& header() { return sImage->header; }

const DirectoryEntry& section(const Section section) { return frameSection(section); }

std::size_t printStem(const std::string_view spelling, const std::uint8_t length, const char* const pattern,
                      char* const out) {
  const bool marked = pattern[0] != '\0';
  assert(length <= spelling.size());
  for (std::size_t i = 0; i < length; ++i)
    out[i] = marked && pattern[i] != '.' ? pattern[i] : facts::foldLetter(spelling[i]);
  return length;
}

} // namespace whitaker::relationship
