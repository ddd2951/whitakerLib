#include "relationship_image.hpp"

#include "facts.hpp"
#include "image.hpp"

#include <algorithm>
#include <cassert>
#include <ranges>

namespace whitaker::relationship {
namespace {

[[nodiscard]] const unsigned char* at(Section section, std::uint64_t index) noexcept {
  return recordAt(*sImage, section, index);
}

[[nodiscard]] std::uint16_t addonCount() noexcept { return static_cast<std::uint16_t>(section(Section::Addons).count); }

[[nodiscard]] const char* stringAt(std::uint32_t offset) noexcept { return sImage->strings + offset; }

template <typename Record>
[[nodiscard, gnu::always_inline]] inline Record read(Section section, std::uint64_t index) noexcept {
  return readRecord<Record>(at(section, index));
}

} // namespace

Program lookup(std::string_view word) noexcept {
  if (word.empty() || word.size() > header().maximumWordLength)
    return {};
  std::uint32_t state = header().root;
  std::uint32_t output = header().rootOutput;
  for (char raw : word) {
    const char c = facts::foldLetter(raw);
    if (c < 'a' || c > 'z')
      return {};
    const StateRecord record = read<StateRecord>(Section::States, state);
    const std::uint32_t bit = 1u << static_cast<unsigned>(c - 'a');
    if ((record.letterMask & bit) == 0)
      return {};
    const TransitionRecord edge = read<TransitionRecord>(
        Section::Transitions, record.firstTransition + popcount32(record.letterMask & (bit - 1)));
    state = edge.target;
    output += edge.output;
  }
  const std::uint8_t count = read<StateRecord>(Section::States, state).resultCount;
  return count == 0 ? Program{} : Program{output, count};
}

Result next(std::uint32_t& cursor, std::uint16_t& lexeme) noexcept {
  const unsigned char* instructions = at(Section::Instructions, 0);
  const std::uint8_t operation = instructions[cursor++];
  if ((operation & kLexemeChange) != 0) {
    lexeme = read16(instructions + cursor);
    cursor += 2;
  }
  const LexemeRecord record = read<LexemeRecord>(Section::Lexemes, lexeme);
  const ParadigmRecord paradigm = read<ParadigmRecord>(Section::Paradigms, record.paradigm);
  const std::uint16_t description =
      read16(at(Section::Targets, paradigm.firstTarget + (operation & kRelationshipMask)));
  const std::uint16_t printed = dictionary(record.dictionary).stems[record.stem];
  return {static_cast<std::uint8_t>(printed & ((1u << kStemLengthBits) - 1)),
          stringAt(read32(at(Section::StemPatterns, printed >> kStemLengthBits))), grammar(description),
          record.dictionary, record.stem};
}

Entry entry(std::uint16_t dictionary) noexcept {
  return decodeEntry(readWord(at(Section::Entries, dictionary), kEntryBytes));
}

DictionaryRecord dictionary(std::uint16_t index) noexcept {
  return read<DictionaryRecord>(Section::Dictionaries, index);
}

const char* dictionaryMeaning(std::uint16_t index) noexcept { return stringAt(dictionary(index).meaning); }

ClassRecord dictionaryClass(std::uint16_t index) noexcept { return read<ClassRecord>(Section::Classes, index); }

latin::Analysis grammar(std::uint16_t description) noexcept {
  return sImage->grammars.empty() ? decodeGrammar(*sImage, description) : sImage->grammars[description];
}

FallbackRowRecord fallbackRow(std::uint32_t index) noexcept {
  return read<FallbackRowRecord>(Section::FallbackRows, index);
}

InflectionRecord inflection(std::uint16_t index) noexcept {
  return read<InflectionRecord>(Section::Inflections, index);
}

FallbackStemRecord fallbackStem(std::uint32_t index) noexcept {
  return read<FallbackStemRecord>(Section::FallbackStems, index);
}

Run fallbackStemRange(std::string_view text) noexcept {
  const auto stems =
      std::views::iota(std::uint32_t{0}, static_cast<std::uint32_t>(section(Section::FallbackStems).count));
  const auto found = std::ranges::equal_range(
      stems, text, {}, [](std::uint32_t i) { return std::string_view{stringAt(fallbackStem(i).text)}; });
  return {static_cast<std::uint32_t>(found.begin() - stems.begin()), static_cast<std::uint32_t>(found.size())};
}

Run endingRun(std::string_view text) noexcept {
  const auto endings = std::views::iota(std::uint32_t{0}, static_cast<std::uint32_t>(section(Section::Endings).count));
  const auto ending = [](std::uint32_t i) { return read<EndingRecord>(Section::Endings, i); };
  const auto found = std::ranges::lower_bound(
      endings, text, {}, [&](std::uint32_t i) { return std::string_view{stringAt(ending(i).text)}; });
  if (found == endings.end() || std::string_view{stringAt(ending(*found).text)} != text)
    return {};
  return ending(*found).inflections();
}

Addon addon(std::uint16_t id) noexcept { return id < addonCount() ? decodeAddon(*sImage, id) : Addon{}; }

std::span<const Addon> addonsOf(latin::AddonKind kind) noexcept {
  return sImage->addonsByKind[std::to_underlying(kind)];
}

const HeaderRecord& header() noexcept { return sImage->header; }

const DirectoryEntry& section(Section section) noexcept { return sectionOf(*sImage, section); }

std::size_t printStem(std::string_view spelling, std::uint8_t length, const char* pattern, char* out) noexcept {
  const bool marked = pattern[0] != '\0';
  assert(length <= spelling.size());
  for (std::size_t i = 0; i < length; ++i)
    out[i] = marked && pattern[i] != '.' ? pattern[i] : facts::foldLetter(spelling[i]);
  return length;
}

} // namespace whitaker::relationship
