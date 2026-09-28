#include "relationship_post_validation.hpp"

#include "error/error.hpp"
#include "entry_values.hpp"
#include "facts.hpp"
#include "rendered_analysis.hpp"
#include "expand/expand.hpp"
#include "src/search/relationship_image.hpp"
#include "word/word.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <limits>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

int failures{};

void check(bool condition, std::string_view message) {
  if (!condition) {
    std::println(stderr, "FAIL: {}", message);
    ++failures;
  }
}

[[nodiscard]] std::vector<std::byte>
readImage(const std::filesystem::path& path) {
  std::ifstream input(path, std::ios::binary | std::ios::ate);
  check(static_cast<bool>(input), "the generated image can be opened");
  if (!input)
    return {};
  const std::streamoff size = input.tellg();
  std::vector<std::byte> bytes(static_cast<std::size_t>(size));
  input.seekg(0);
  input.read(reinterpret_cast<char*>(bytes.data()), size);
  check(static_cast<bool>(input), "the generated image reads completely");
  return bytes;
}

void reject(std::vector<std::byte> bytes, std::string_view message,
            std::string_view reason) {
  rel::Image image;
  std::string failure;
  const bool loaded = image.loadBytes(std::move(bytes), failure);
  check(!loaded, message);
  check(failure == reason, message);
  check(!image.valid(), "a rejected image is not valid");
}

void write32(std::vector<std::byte>& bytes, std::size_t at,
             std::uint32_t value) {
  for (unsigned shift = 0; shift < 32; shift += 8)
    bytes[at + shift / 8] = static_cast<std::byte>(value >> shift);
}

struct ExpectedRow {
  std::string word;
  expand::ResultIndex result;
  std::uint32_t sourceOrder{};
};

void validateSemanticRows(const rel::Image& image,
                          const std::vector<std::string>& spellings) {
  std::vector<ExpectedRow> expected;
  expected.reserve(expand::resultCount());
  std::uint32_t sourceOrder = 0;
  for (std::size_t index = 0; index < expand::resultCount(); ++index) {
    const expand::ResultIndex result{index};
    if (expand::Result{result}.fate() != expand::Fate::kept)
      continue;
    expected.push_back(
        ExpectedRow{emitter::normalize(expand::Result{result}.spelling()),
                    result, sourceOrder++});
  }

  std::ranges::sort(
      expected, [](const ExpectedRow& left, const ExpectedRow& right) {
        return left.word != right.word ? left.word < right.word
                                       : left.sourceOrder < right.sourceOrder;
      });
  if (expected.size() != image.resultCount()) {
    check(false, "the image contains every kept semantic row");
    return;
  }

  std::size_t spellingOrdinal = 0;
  std::size_t checkedRows = 0;
  for (std::size_t row = 0; row < expected.size();) {
    const std::size_t begin = row;
    while (row < expected.size() && expected[row].word == expected[begin].word)
      ++row;
    const std::size_t count = row - begin;
    if (spellingOrdinal >= spellings.size() ||
        spellings[spellingOrdinal] != expected[begin].word) {
      check(false, "the image contains every kept spelling in canonical order");
      return;
    }

    const rel::Image::Program program = image.lookup(expected[begin].word);
    if (program.count != count) {
      check(false, "a spelling keeps every semantic row in canonical order");
      return;
    }
    std::uint32_t cursor = program.begin;
    std::uint16_t dense = std::numeric_limits<std::uint16_t>::max();
    for (std::size_t index = begin; index < row; ++index) {
      const rel::Image::Result actual = image.next(cursor, dense);
      if (actual.orth == nullptr || actual.meaning == nullptr) {
        check(false, "every rendered field resolves into the string pool");
        return;
      }
      const emitter::Rendered rendered =
          emitter::render(expand::Result{expected[index].result}.reading());
      if (rendered.orth != actual.orth || rendered.meaning != actual.meaning ||
          rendered.analysis != actual.grammar) {
        check(false, "every ordered row retains its orth, meaning and grammar");
        return;
      }
      ++checkedRows;
    }
    ++spellingOrdinal;
  }

  check(spellingOrdinal == spellings.size(),
        "the image contains no spelling absent from the semantic facts");
  check(checkedRows == image.resultCount(),
        "the exhaustive semantic row count matches the image");
}

void validateEntries(const rel::Image& image) {
  const std::uint32_t count = word::kDictlineWords + expand::kUniquesRows;
  if (image.section(rel::Section::Entries).count != count) {
    check(false, "the entries section covers every parsed word");
    return;
  }
  for (std::uint32_t index = 0; index < count; ++index) {
    rel::Entry expected;
    if (index < word::kDictlineWords) {
      const word::Dictline word{word::DictlineRow{index}};
      expected = emitter::entryValues(word.grammar(), word.area(),
                                      word.geography(), word.source());
    } else {
      const word::Uniques word{
          source::UniquesIndex{index - word::kDictlineWords}};
      expected = emitter::entryValues(word.grammar(), word.area(),
                                      word.geography(), word.source());
    }
    if (image.entry(index) != expected) {
      check(false, "every stored entry matches its parsed word");
      return;
    }
  }
  std::println("  entries: {} parsed words round-trip exactly", count);
}

} // namespace

void emitter::validatePublishedRelationshipImage(
    const std::filesystem::path& path) {
  failures = 0;
  const std::vector<std::byte> source = readImage(path);
  check(!source.empty(), "the generated image has bytes");
  if (failures != 0 || source.empty())
    error::fatal("relationship post validation could not read the image");

  rel::Image accepted;
  std::string failure;
  check(accepted.loadBytes(source, failure), "the generated image is accepted");
  if (!accepted.valid()) {
    std::println(stderr, "FAIL: {}", failure);
    error::fatal("relationship post validation rejected the image");
  }
  check(failure.empty(), "an accepted image reports no failure");
  check(accepted.bytes() == source.size(), "the image keeps every byte");
  check(accepted.maximumWordLength() <= rel::kMaximumWordLength,
        "the header's spelling length is within the format's capacity");

  const std::vector<std::string> spellings = accepted.spellings();
  check(spellings.size() == accepted.spellingCount(),
        "the walk finds every spelling the header counts");
  validateSemanticRows(accepted, spellings);
  validateEntries(accepted);
  check(accepted.lookup("").count == 0, "the empty word matches nothing");
  check(accepted.lookup("qqqqqqqqqqqqqqqqqqqqqqqqqqqq").count == 0,
        "a word longer than the header allows matches nothing");

  {
    auto b = source;
    b[0] ^= std::byte{1};
    reject(std::move(b), "a wrong magic is refused",
           "invalid relationship header");
  }
  {
    auto b = source;
    write32(b, 8, rel::kVersion + 1);
    reject(std::move(b), "a wrong version is refused",
           "invalid relationship header");
  }
  {
    auto b = source;
    write32(b, 12, 44);
    reject(std::move(b), "a wrong header size is refused",
           "invalid relationship header");
  }
  {
    auto b = source;
    write32(b, 24, rel::kSectionCount - 1);
    reject(std::move(b), "a wrong section count is refused",
           "invalid relationship header");
  }

  {
    auto b = source;
    write32(b, 36, rel::kMaximumWordLength + 1);
    reject(std::move(b), "a spelling length past capacity is refused",
           "invalid relationship header counts");
  }
  {
    auto b = source;
    write32(b, 36, accepted.maximumWordLength() - 1);
    reject(std::move(b), "a spelling length the image exceeds is refused",
           "relationship path exceeds image bounds");
  }

  {
    auto b = source;
    b[rel::kHeaderBytes] ^= std::byte{1};
    reject(std::move(b), "a moved section is refused",
           "invalid relationship section ordering or bounds");
  }

  const std::size_t states = accepted.section(rel::Section::States).offset;
  {
    auto b = source;
    b[states + 9] = std::byte{1};
    reject(std::move(b), "a used reserved state byte is refused",
           "invalid relationship state");
  }

  const std::size_t transitions =
      accepted.section(rel::Section::Transitions).offset;
  {
    auto b = source;
    write32(b, transitions, 0xffffffffu);
    reject(std::move(b), "a cyclic transition is refused",
           "relationship transition target is not acyclic");
  }

  const std::size_t instructions =
      accepted.section(rel::Section::Instructions).offset;
  {
    auto b = source;
    b[instructions] &= std::byte{rel::kRelationshipMask};
    reject(std::move(b), "a program that selects no lexeme is refused",
           "first result does not select a dense lexeme");
  }

  const rel::DirectoryEntry& strings = accepted.section(rel::Section::Strings);
  {
    auto b = source;
    b[strings.offset + strings.bytes - 1] = std::byte{'x'};
    reject(std::move(b), "an unterminated string pool is refused",
           "invalid inflection row");
  }

  const std::size_t grammars = accepted.section(rel::Section::Grammars).offset;
  {
    auto b = source;
    b[grammars] = std::byte{14};
    reject(std::move(b), "a grammar with no part is refused",
           "invalid description grammar");
  }
  {
    auto b = source;
    b[grammars + 3] = std::byte{200};
    reject(std::move(b), "a grammar with an unknown case is refused",
           "invalid description grammar");
  }

  const std::size_t entries = accepted.section(rel::Section::Entries).offset;
  {
    auto b = source;
    b[entries] = std::byte{0};
    reject(std::move(b), "an entry with no part is refused",
           "invalid dictionary entry part");
  }
  {
    auto b = source;
    b[entries + 7] = std::byte{200};
    reject(std::move(b), "an entry with an unknown area is refused",
           "invalid dictionary entry field");
  }

  {
    std::uint32_t tackon = 0, prefix = 0;
    for (std::uint16_t index = 0; index < accepted.addonCount(); ++index) {
      const rel::Image::Addon addon = accepted.addon(index);
      if (addon.kind == latin::AddonKind::Tackon && tackon == 0)
        tackon = std::uint32_t{index} + 1;
      if (addon.kind == latin::AddonKind::Prefix && prefix == 0 &&
          std::string_view{addon.fix}.size() == 1)
        prefix = std::uint32_t{index} + 1;
    }
    check(tackon != 0 && prefix != 0,
          "the image has a tackon and a one-letter prefix to corrupt with");
    const std::size_t addons = accepted.section(rel::Section::Addons).offset;
    auto b = source;
    write32(b, addons + (tackon - 1) * rel::kAddonBytes,
            rel::read32(b.data() + addons + (prefix - 1) * rel::kAddonBytes));
    reject(std::move(b), "a one-letter tackon is refused",
           "ADDONS fix too short for the step bound");
  }

  if (failures != 0) {
    std::println(stderr, "{} failure(s)", failures);
    error::fatal("relationship post validation failed");
  }
  std::println("  exhaustive spelling / ordered-row / field check: exact");
}
