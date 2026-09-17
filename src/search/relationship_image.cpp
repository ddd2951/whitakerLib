#include "relationship_image.hpp"

#include "facts.hpp"

#include <algorithm>
#include <bit>
#include <cstring>
#ifndef WHITAKER_EMBEDDED_ONLY
#include <fstream>
#endif
#include <limits>
#include <ranges>
#include <utility>
#include <vector>

#ifndef WHITAKER_EMBEDDED_ONLY
namespace fs = std::filesystem;
#endif

namespace {

[[nodiscard]] bool fail(std::string& failure, std::string message) {
  failure = std::move(message);
  return false;
}

} // namespace

namespace whitaker::relationship {

const DirectoryEntry& Image::section(Section section) const noexcept {
  return m_directory[static_cast<std::size_t>(section)];
}

const std::byte* Image::at(Section id, std::uint64_t index,
                           std::size_t width) const noexcept {
  return m_bytes.data() + section(id).offset + index * width;
}

const char* Image::stringAt(std::uint32_t offset) const noexcept {
  return reinterpret_cast<const char*>(
      m_bytes.data() + section(Section::Strings).offset + offset);
}

#ifndef WHITAKER_EMBEDDED_ONLY
bool Image::load(const fs::path& path, std::string& failure) {
  m_valid = false;
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  if (!input)
    return fail(failure, "cannot open relationship image");
  const std::streamoff end = input.tellg();
  if (end <= 0)
    return fail(failure, "relationship image is empty");
  std::vector<std::byte> bytes(static_cast<std::size_t>(end));
  input.seekg(0);
  input.read(reinterpret_cast<char*>(bytes.data()), end);
  if (!input)
    return fail(failure, "cannot read complete relationship image");
  return loadBytes(std::move(bytes), failure);
}

bool Image::loadBytes(std::vector<std::byte> bytes, std::string& failure) {
  m_ownedBytes = std::move(bytes);
  return loadSpan(m_ownedBytes, failure);
}
#endif

bool Image::loadSpan(std::span<const std::byte> bytes, std::string& failure) {
  m_valid = false;
  m_bytes = bytes;
  if (!validate(failure)) {
    m_bytes = {};
#ifndef WHITAKER_EMBEDDED_ONLY
    m_ownedBytes.clear();
#endif
    m_directory = {};
    return false;
  }
  m_valid = true;
  failure.clear();
  return true;
}

bool Image::validateString(std::uint32_t offset, std::string& failure) const {
  const DirectoryEntry& pool = section(Section::Strings);
  if (offset >= pool.bytes || std::memchr(m_bytes.data() + pool.offset + offset,
                                          '\0', pool.bytes - offset) == nullptr)
    return fail(failure, "invalid relationship string offset or terminator");
  return true;
}

bool Image::validateProgram(std::uint32_t begin, std::uint8_t count,
                            std::uint32_t& end, std::string& failure) const {
  const DirectoryEntry& programs = section(Section::Instructions);
  if (begin >= programs.bytes || count == 0)
    return fail(failure, "invalid relationship instruction start or count");
  std::uint32_t cursor = begin;
  std::uint16_t dense = std::numeric_limits<std::uint16_t>::max();
  for (std::uint32_t row = 0; row < count; ++row) {
    if (cursor >= programs.bytes)
      return fail(failure, "relationship instruction exceeds section");
    const std::uint8_t operation =
        std::to_integer<std::uint8_t>(*at(Section::Instructions, cursor++, 1));
    if ((operation & kLexemeChange) != 0) {
      if (programs.bytes - cursor < 2)
        return fail(failure, "truncated dense lexeme selection");
      dense = read16(at(Section::Instructions, cursor, 1));
      cursor += 2;
      if (dense >= section(Section::Lexemes).count)
        return fail(failure, "dense lexeme ID is out of bounds");
    } else if (row == 0) {
      return fail(failure, "first result does not select a dense lexeme");
    }
    const std::byte* lexeme = at(Section::Lexemes, dense, kLexemeBytes);
    const std::byte* paradigm =
        at(Section::Paradigms, read16(lexeme + 4), kParadigmBytes);
    if ((operation & kRelationshipMask) >=
        std::to_integer<std::uint8_t>(paradigm[4]))
      return fail(failure, "relationship ID is out of bounds");
  }
  end = cursor;
  return true;
}

bool Image::validateHeader(std::string& failure) {
  if (m_bytes.size() < kHeaderBytes + kSectionCount * kDirectoryBytes ||
      std::memcmp(m_bytes.data(), kMagic.data(), kMagic.size()) != 0 ||
      read32(m_bytes.data() + 8) != kVersion ||
      read32(m_bytes.data() + 12) != kHeaderBytes ||
      read64(m_bytes.data() + 16) != m_bytes.size() ||
      read32(m_bytes.data() + 24) != kSectionCount)
    return fail(failure, "invalid relationship header");

  m_root = read32(m_bytes.data() + 28);
  m_rootOutput = read32(m_bytes.data() + 32);
  m_maximumWordLength = read32(m_bytes.data() + 36);
  m_spellingCount = read32(m_bytes.data() + 40);
  m_resultCount = read32(m_bytes.data() + 44);
  if (m_maximumWordLength == 0 || m_maximumWordLength > kMaximumWordLength ||
      m_spellingCount == 0 || m_resultCount == 0)
    return fail(failure, "invalid relationship header counts");
  return true;
}

bool Image::validateDirectory(std::string& failure) {
  std::uint64_t cursor = kHeaderBytes + kSectionCount * kDirectoryBytes;
  for (std::size_t i = 0; i < m_directory.size(); ++i) {
    const std::byte* record =
        m_bytes.data() + kHeaderBytes + i * kDirectoryBytes;
    m_directory[i] = {read64(record), read64(record + 8), read64(record + 16)};
    const DirectoryEntry& entry = m_directory[i];
    if (entry.offset != cursor || entry.offset > m_bytes.size() ||
        entry.bytes > m_bytes.size() - entry.offset)
      return fail(failure, "invalid relationship section ordering or bounds");
    cursor += entry.bytes;
  }
  if (cursor != m_bytes.size())
    return fail(failure, "relationship sections do not cover image");

  const auto exactWidth = [this](Section id, std::uint64_t width) {
    const DirectoryEntry& entry = section(id);
    return entry.count <= std::numeric_limits<std::uint64_t>::max() / width &&
           entry.bytes == entry.count * width;
  };
  if (!exactWidth(Section::States, kStateBytes) ||
      !exactWidth(Section::Transitions, kTransitionBytes) ||
      !exactWidth(Section::Lexemes, kLexemeBytes) ||
      !exactWidth(Section::Paradigms, kParadigmBytes) ||
      !exactWidth(Section::Targets, kTargetBytes) ||
      !exactWidth(Section::Dictionaries, kDictionaryBytes) ||
      !exactWidth(Section::Descriptions, kDescriptionBytes) ||
      !exactWidth(Section::Addons, kAddonBytes) ||
      !exactWidth(Section::Classes, kClassBytes) ||
      !exactWidth(Section::FallbackRows, kFallbackRowBytes) ||
      !exactWidth(Section::Inflections, kInflectionBytes) ||
      !exactWidth(Section::Endings, kEndingBytes) ||
      !exactWidth(Section::FallbackStems, kFallbackStemBytes) ||
      section(Section::Instructions).count != m_resultCount ||
      m_root >= section(Section::States).count ||
      m_rootOutput >= section(Section::Instructions).bytes)
    return fail(failure, "invalid relationship section width or root");
  return true;
}

bool Image::validateStates(std::string& failure) const {
  std::uint64_t expectedTransition = 0;
  for (std::uint32_t state = 0; state < section(Section::States).count;
       ++state) {
    const std::byte* record = at(Section::States, state, kStateBytes);
    const std::uint32_t first = read32(record);
    const std::uint32_t mask = read32(record + 4);
    const std::uint8_t count = std::to_integer<std::uint8_t>(record[8]);
    const std::uint32_t edges = popcount32(mask);
    if ((mask & ~kLetterMask) != 0 || count > kMaximumResultsPerSpelling ||
        record[9] != std::byte{} || read16(record + 10) != 0 ||
        first != expectedTransition ||
        first > section(Section::Transitions).count ||
        edges > section(Section::Transitions).count - first)
      return fail(failure, "invalid relationship state");
    for (std::uint32_t i = 0; i < edges; ++i) {
      const std::byte* edge =
          at(Section::Transitions, first + i, kTransitionBytes);
      if (read32(edge) >= state)
        return fail(failure, "relationship transition target is not acyclic");
    }
    expectedTransition += edges;
  }
  if (expectedTransition != section(Section::Transitions).count)
    return fail(failure, "relationship states do not cover transitions");
  return true;
}

bool Image::validateLexemes(std::string& failure) const {
  std::uint32_t previousSparse = 0;
  for (std::uint32_t dense = 0; dense < section(Section::Lexemes).count;
       ++dense) {
    const std::byte* record = at(Section::Lexemes, dense, kLexemeBytes);
    const std::uint16_t dictionary = read16(record);
    const std::uint8_t stem = std::to_integer<std::uint8_t>(record[2]);
    const std::uint32_t sparse =
        dictionary | (static_cast<std::uint32_t>(stem) << 16);
    if (dictionary >= section(Section::Dictionaries).count ||
        stem >= facts::kStemColumnCount ||
        (dense != 0 && sparse <= previousSparse) || record[3] != std::byte{} ||
        read16(record + 4) >= section(Section::Paradigms).count ||
        read16(record + 6) != 0)
      return fail(failure, "invalid dense lexeme mapping");
    previousSparse = sparse;
  }
  return true;
}

bool Image::validateParadigms(std::string& failure) const {
  std::uint64_t expectedTarget = 0;
  for (std::uint32_t id = 0; id < section(Section::Paradigms).count; ++id) {
    const std::byte* record = at(Section::Paradigms, id, kParadigmBytes);
    const std::uint32_t first = read32(record);
    const std::uint8_t count = std::to_integer<std::uint8_t>(record[4]);
    if (first != expectedTarget || count == 0 ||
        count > kMaximumTargetsPerParadigm || record[5] != std::byte{} ||
        read16(record + 6) != 0 || first > section(Section::Targets).count ||
        count > section(Section::Targets).count - first)
      return fail(failure, "invalid paradigm target range");
    std::uint16_t previous = 0;
    for (std::uint32_t local = 0; local < count; ++local) {
      const std::uint16_t target =
          read16(at(Section::Targets, first + local, kTargetBytes));
      if (target >= section(Section::Descriptions).count ||
          (local != 0 && target <= previous))
        return fail(failure, "invalid paradigm description target");
      previous = target;
    }
    expectedTarget += count;
  }
  if (expectedTarget != section(Section::Targets).count)
    return fail(failure, "paradigms do not cover description targets");
  return true;
}

bool Image::validateRecords(std::string& failure) const {
  for (std::uint32_t dictionary = 0;
       dictionary < section(Section::Dictionaries).count; ++dictionary) {
    const std::byte* record =
        at(Section::Dictionaries, dictionary, kDictionaryBytes);
    for (unsigned field = 0; field < 5; ++field)
      if (!validateString(read32(record + field * 4), failure))
        return false;
  }
  for (std::uint32_t description = 0;
       description < section(Section::Descriptions).count; ++description) {
    const std::byte* record =
        at(Section::Descriptions, description, kDescriptionBytes);
    if (!validateString(read32(record), failure) ||
        !validateString(read32(record + 4), failure))
      return false;
  }
  for (std::uint32_t addon = 0; addon < section(Section::Addons).count;
       ++addon) {
    const std::byte* record = at(Section::Addons, addon, kAddonBytes);
    if (!validateString(read32(record), failure) ||
        !validateString(read32(record + 4), failure) ||
        !isAddonKind(std::to_integer<std::uint8_t>(record[16])) ||
        !isPart(std::to_integer<std::uint8_t>(record[17])) ||
        !isPart(std::to_integer<std::uint8_t>(record[18])) ||
        std::to_integer<std::uint8_t>(record[19]) > 9 ||
        std::to_integer<std::uint8_t>(record[20]) > 9 ||
        std::uint64_t{read32(record + 23)} + read16(record + 27) >
            section(Section::FallbackRows).count ||
        read16(record + 30) != 0)
      return fail(failure, "invalid ADDONS record");
  }
  for (std::uint32_t row = 0; row < section(Section::FallbackRows).count;
       ++row) {
    const std::byte* record = at(Section::FallbackRows, row, kFallbackRowBytes);
    if (read16(record) >= section(Section::Inflections).count ||
        read16(record + 2) >= section(Section::Descriptions).count)
      return fail(failure, "invalid fallback row");
  }
  for (std::uint32_t id = 0; id < section(Section::Classes).count; ++id) {
    const std::byte* record = at(Section::Classes, id, kClassBytes);
    if (std::uint64_t{read32(record)} + read16(record + 4) >
        section(Section::FallbackRows).count)
      return fail(failure, "invalid dictionary class");
  }
  for (std::uint32_t row = 0; row < section(Section::Inflections).count;
       ++row) {
    const std::byte* record = at(Section::Inflections, row, kInflectionBytes);
    if (!validateString(read32(record), failure) ||
        std::to_integer<std::uint8_t>(record[4]) > 9)
      return fail(failure, "invalid inflection row");
  }
  const char* previousEnding = nullptr;
  for (std::uint32_t index = 0; index < section(Section::Endings).count;
       ++index) {
    const std::byte* record = at(Section::Endings, index, kEndingBytes);
    if (!validateString(read32(record), failure) ||
        std::uint64_t{read16(record + 4)} + read16(record + 6) >
            section(Section::Inflections).count)
      return fail(failure, "invalid ending run");
    const char* ending = stringAt(read32(record));
    if (previousEnding != nullptr &&
        !(std::string_view{previousEnding} < std::string_view{ending}))
      return fail(failure, "endings are not ordered");
    previousEnding = ending;
  }
  for (std::uint32_t stem = 0; stem < section(Section::FallbackStems).count;
       ++stem) {
    const std::byte* record =
        at(Section::FallbackStems, stem, kFallbackStemBytes);
    if (!validateString(read32(record), failure) ||
        read16(record + 4) >= section(Section::Dictionaries).count ||
        std::to_integer<std::uint8_t>(record[6]) > 9 ||
        std::to_integer<std::uint8_t>(record[7]) >
            static_cast<std::uint8_t>(Part::NONE))
      return fail(failure, "invalid ADDONS fallback stem");
  }
  return true;
}

bool Image::validateStringPool(std::string& failure) const {
  const DirectoryEntry& strings = section(Section::Strings);
  if (strings.bytes == 0 || m_bytes[strings.offset] != std::byte{} ||
      m_bytes[strings.offset + strings.bytes - 1] != std::byte{})
    return fail(failure, "invalid relationship string pool boundary");
  std::uint64_t terminators = 0;
  for (std::uint64_t i = 0; i < strings.bytes; ++i)
    terminators += m_bytes[strings.offset + i] == std::byte{};
  if (terminators != strings.count)
    return fail(failure, "relationship string count differs from pool");
  return true;
}

bool Image::validateWalk(std::string& failure) const {
  struct Visit {
    std::uint32_t state;
    std::uint32_t output;
    std::uint8_t depth;
  };
  std::vector<Visit> pending{{m_root, m_rootOutput, 0}};
  std::vector<std::pair<std::uint32_t, std::uint32_t>> programs;
  programs.reserve(m_spellingCount);
  std::uint64_t observedResults = 0;
  std::uint32_t observedMaximumLength = 0;
  while (!pending.empty()) {
    const Visit visit = pending.back();
    pending.pop_back();
    const std::byte* state = at(Section::States, visit.state, kStateBytes);
    const std::uint8_t count = std::to_integer<std::uint8_t>(state[8]);
    if (count != 0) {
      std::uint32_t end = 0;
      if (!validateProgram(visit.output, count, end, failure))
        return false;
      programs.emplace_back(visit.output, end);
      observedResults += count;
      observedMaximumLength = std::max(observedMaximumLength,
                                       static_cast<std::uint32_t>(visit.depth));
    }
    const std::uint32_t first = read32(state);
    const std::uint32_t edges = popcount32(read32(state + 4));
    for (std::uint32_t i = 0; i < edges; ++i) {
      const std::byte* edge =
          at(Section::Transitions, first + i, kTransitionBytes);
      const std::uint32_t delta = read32(edge + 4);
      if (visit.output > std::numeric_limits<std::uint32_t>::max() - delta ||
          visit.depth >= m_maximumWordLength)
        return fail(failure, "relationship path exceeds image bounds");
      pending.push_back(Visit{read32(edge), visit.output + delta,
                              static_cast<std::uint8_t>(visit.depth + 1)});
    }
  }
  if (programs.size() != m_spellingCount || observedResults != m_resultCount ||
      observedMaximumLength != m_maximumWordLength)
    return fail(failure, "relationship terminal counts differ from header");
  std::ranges::sort(programs);
  std::uint32_t next = 0;
  for (const auto [begin, end] : programs) {
    if (begin != next || end < begin)
      return fail(failure, "relationship programs overlap or leave a gap");
    next = end;
  }
  if (next != section(Section::Instructions).bytes)
    return fail(failure, "relationship programs do not cover instructions");
  return true;
}

bool Image::validate(std::string& failure) {
  return validateHeader(failure) && validateDirectory(failure) &&
         validateStates(failure) && validateLexemes(failure) &&
         validateParadigms(failure) && validateRecords(failure) &&
         validateStringPool(failure) && validateWalk(failure);
}

Image::Program Image::lookup(std::string_view word) const noexcept {
  if (!m_valid || word.empty() || word.size() > m_maximumWordLength)
    return {};
  std::uint32_t state = m_root;
  std::uint32_t output = m_rootOutput;
  for (char raw : word) {
    const char c = facts::foldLetter(raw);
    if (c < 'a' || c > 'z')
      return {};
    const std::byte* record = at(Section::States, state, kStateBytes);
    const std::uint32_t mask = read32(record + 4);
    const std::uint32_t bit = 1u << static_cast<unsigned>(c - 'a');
    if ((mask & bit) == 0)
      return {};
    const std::uint32_t edgeIndex =
        read32(record) + popcount32(mask & (bit - 1));
    const std::byte* edge =
        at(Section::Transitions, edgeIndex, kTransitionBytes);
    state = read32(edge);
    output += read32(edge + 4);
  }
  const std::uint8_t count =
      std::to_integer<std::uint8_t>(at(Section::States, state, kStateBytes)[8]);
  return count == 0 ? Program{} : Program{output, count};
}

Image::Result Image::next(std::uint32_t& cursor,
                          std::uint16_t& denseLexeme) const noexcept {
  const std::byte* instructions =
      m_bytes.data() + section(Section::Instructions).offset;
  const std::uint8_t operation =
      std::to_integer<std::uint8_t>(instructions[cursor++]);
  if ((operation & kLexemeChange) != 0) {
    denseLexeme = read16(instructions + cursor);
    cursor += 2;
  }
  const std::byte* lexeme = at(Section::Lexemes, denseLexeme, kLexemeBytes);
  const std::byte* paradigm =
      at(Section::Paradigms, read16(lexeme + 4), kParadigmBytes);
  const std::uint16_t description = read16(
      at(Section::Targets, read32(paradigm) + (operation & kRelationshipMask),
         kTargetBytes));
  const std::byte* dictionary =
      at(Section::Dictionaries, read16(lexeme), kDictionaryBytes);
  const std::byte* rendered =
      at(Section::Descriptions, description, kDescriptionBytes);
  const std::uint8_t stem = std::to_integer<std::uint8_t>(lexeme[2]);
  return {stringAt(read32(dictionary + stem * 4)),
          stringAt(read32(dictionary + 16)), stringAt(read32(rendered)),
          stringAt(read32(rendered + 4))};
}

std::uint32_t Image::addonCount() const noexcept {
  return static_cast<std::uint32_t>(section(Section::Addons).count);
}

Image::Addon Image::addon(std::uint32_t index) const noexcept {
  if (!m_valid || index >= addonCount())
    return {};
  const std::byte* record = at(Section::Addons, index, kAddonBytes);
  return {
      .fix = stringAt(read32(record)),
      .meaning = stringAt(read32(record + 4)),
      .target = {read16(record + 8), read16(record + 10), read16(record + 12),
                 read16(record + 14)},
      .kind = static_cast<AddonKind>(std::to_integer<std::uint8_t>(record[16])),
      .root = static_cast<Part>(std::to_integer<std::uint8_t>(record[17])),
      .targetPart =
          static_cast<Part>(std::to_integer<std::uint8_t>(record[18])),
      .rootKey = std::to_integer<std::uint8_t>(record[19]),
      .targetKey = std::to_integer<std::uint8_t>(record[20]),
      .connect = static_cast<char>(std::to_integer<std::uint8_t>(record[21])),
      .targetGate = std::to_integer<std::uint8_t>(record[22]),
      .firstRaw = static_cast<char>(std::to_integer<std::uint8_t>(record[29])),
      .rowStart = read32(record + 23),
      .rowCount = read16(record + 27),
  };
}

Image::FallbackRow Image::fallbackRow(std::uint32_t index) const noexcept {
  if (!m_valid || index >= section(Section::FallbackRows).count)
    return {};
  const std::byte* record = at(Section::FallbackRows, index, kFallbackRowBytes);
  return {.inflect = read16(record), .description = read16(record + 2)};
}

Image::Class Image::dictionaryClass(std::uint16_t index) const noexcept {
  if (!m_valid || index >= section(Section::Classes).count)
    return {};
  const std::byte* record = at(Section::Classes, index, kClassBytes);
  return {.rowStart = read32(record),
          .rowCount = read16(record + 4),
          .gate = read16(record + 6)};
}

const char* Image::dictionaryMeaning(std::uint16_t index) const noexcept {
  if (!m_valid || index >= section(Section::Dictionaries).count)
    return nullptr;
  return stringAt(read32(at(Section::Dictionaries, index, kDictionaryBytes) +
                         facts::kStemColumnCount * 4));
}

Image::Inflection Image::inflection(std::uint16_t index) const noexcept {
  if (!m_valid || index >= section(Section::Inflections).count)
    return {};
  const std::byte* record = at(Section::Inflections, index, kInflectionBytes);
  return {.ending = stringAt(read32(record)),
          .key = std::to_integer<std::uint8_t>(record[4]),
          .allow = std::to_integer<std::uint8_t>(record[5]),
          .age = std::to_integer<std::uint8_t>(record[6]),
          .frequency = std::to_integer<std::uint8_t>(record[7])};
}

Image::Description Image::description(std::uint16_t index) const noexcept {
  if (!m_valid || index >= section(Section::Descriptions).count)
    return {};
  const std::byte* record = at(Section::Descriptions, index, kDescriptionBytes);
  return {.pos = stringAt(read32(record)),
          .inflection = stringAt(read32(record + 4))};
}

std::pair<std::uint16_t, std::uint16_t>
Image::endingRun(std::string_view text) const noexcept {
  if (!m_valid)
    return {};
  std::uint32_t first = 0,
                last =
                    static_cast<std::uint32_t>(section(Section::Endings).count);
  while (first < last) {
    const std::uint32_t middle = first + (last - first) / 2;
    const std::byte* record = at(Section::Endings, middle, kEndingBytes);
    const std::string_view candidate{stringAt(read32(record))};
    if (candidate < text)
      first = middle + 1;
    else if (text < candidate)
      last = middle;
    else
      return {read16(record + 4), read16(record + 6)};
  }
  return {};
}

std::uint32_t Image::fallbackStemCount() const noexcept {
  return static_cast<std::uint32_t>(section(Section::FallbackStems).count);
}

Image::FallbackStem Image::fallbackStem(std::uint32_t index) const noexcept {
  if (!m_valid || index >= fallbackStemCount())
    return {};
  const std::byte* record =
      at(Section::FallbackStems, index, kFallbackStemBytes);
  return {.text = stringAt(read32(record)),
          .dictionary = read16(record + 4),
          .key = std::to_integer<std::uint8_t>(record[6]),
          .part = static_cast<Part>(std::to_integer<std::uint8_t>(record[7]))};
}

std::pair<std::uint32_t, std::uint32_t>
Image::fallbackStemRange(std::string_view text) const noexcept {
  const std::uint32_t count = fallbackStemCount();
  const auto lower = [this, count](std::string_view value) {
    std::uint32_t first = 0, last = count;
    while (first < last) {
      const std::uint32_t middle = first + (last - first) / 2;
      const std::byte* record =
          at(Section::FallbackStems, middle, kFallbackStemBytes);
      if (std::string_view{stringAt(read32(record))} < value)
        first = middle + 1;
      else
        last = middle;
    }
    return first;
  };
  const std::uint32_t first = lower(text);
  if (first == count || std::string_view{fallbackStem(first).text} != text)
    return {first, first};
  std::uint32_t last = first + 1;
  while (last < count && std::string_view{fallbackStem(last).text} == text)
    ++last;
  return {first, last};
}

Image::DictionaryMetadata
Image::dictionaryMetadata(std::uint16_t index) const noexcept {
  if (!m_valid || index >= section(Section::Dictionaries).count)
    return {};
  const std::byte* record = at(Section::Dictionaries, index, kDictionaryBytes);
  return {.age = std::to_integer<std::uint8_t>(record[20]),
          .frequency = std::to_integer<std::uint8_t>(record[21]),
          .classId = read16(record + 22)};
}

#ifndef WHITAKER_EMBEDDED_ONLY
std::vector<std::string> Image::spellings() const {
  struct Visit {
    std::uint32_t state;
    std::string text;
  };
  std::vector<std::string> result;
  if (!m_valid)
    return result;
  result.reserve(m_spellingCount);
  std::vector<Visit> pending{{m_root, {}}};
  while (!pending.empty()) {
    Visit visit = std::move(pending.back());
    pending.pop_back();
    const std::byte* state = at(Section::States, visit.state, kStateBytes);
    if (std::to_integer<std::uint8_t>(state[8]) != 0)
      result.push_back(visit.text);
    const std::uint32_t first = read32(state);
    const std::uint32_t mask = read32(state + 4);
    std::uint32_t ordinal = 0;
    for (unsigned letter = 0; letter < facts::kLetterCount; ++letter) {
      if ((mask & (1u << letter)) == 0)
        continue;
      const std::byte* edge =
          at(Section::Transitions, first + ordinal, kTransitionBytes);
      pending.push_back(
          Visit{read32(edge), visit.text + static_cast<char>('a' + letter)});
      ++ordinal;
    }
  }
  std::ranges::sort(result);
  return result;
}
#endif

} // namespace whitaker::relationship
