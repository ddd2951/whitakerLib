#include "relationship_post_validation.hpp"

#include "error/error.hpp"
#include "facts.hpp"
#include "gen/emitter/wl_stable/detail/rendered_analysis.hpp"
#include "gen/expand/wl_stable/facts/forms.hpp"
#include "gen/expand/wl_stable/facts/readings.hpp"
#include "gen/expand/wl_stable/facts/swept.hpp"
#include "src/search/relationship_image.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace rel = whitaker::relationship;
namespace render = wl_stable::emitter::detail::rendered_analysis;

namespace {

int failures{};

void check(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
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

void reject(std::vector<std::byte> bytes, std::string_view message) {
  rel::Image image;
  std::string failure;
  const bool loaded = image.loadBytes(std::move(bytes), failure);
  check(!loaded, message);
  check(!failure.empty(), "a rejected image names its reason");
  check(!image.valid(), "a rejected image is not valid");
}

void write32(std::vector<std::byte>& bytes, std::size_t at,
             std::uint32_t value) {
  for (unsigned shift = 0; shift < 32; shift += 8)
    bytes[at + shift / 8] = static_cast<std::byte>(value >> shift);
}

[[nodiscard]] std::string normalize(std::string_view text) {
  std::string result{text};
  for (char& c : result)
    c = facts::foldLetter(c);
  return result;
}

struct ExpectedRow {
  std::string word;
  const wl_stable::expand::Reading* reading{};
  std::uint32_t sourceOrder{};
};

void validateSemanticRows(const rel::Image& image,
                          const wl_stable::expand::Forms& forms,
                          const wl_stable::expand::Readings& readings,
                          const wl_stable::expand::Swept& swept,
                          const std::vector<std::string>& spellings) {
  std::size_t formCount = 0;
  for (const auto& byLetter : forms.byLetter)
    formCount += byLetter.size();

  std::vector<ExpectedRow> expected;
  expected.reserve(formCount);
  std::uint32_t sourceOrder = 0;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const auto& listed = forms.byLetter[letter];
    const auto& read = readings.byLetter[letter];
    const auto& fates = swept.byLetter[letter];
    if (listed.size() != read.size() || listed.size() != fates.size()) {
      check(false, "the semantic facts remain aligned for post validation");
      return;
    }
    for (std::size_t index = 0; index < listed.size(); ++index) {
      if (fates[index] != wl_stable::expand::Fate::kept)
        continue;
      expected.push_back(ExpectedRow{normalize(listed[index].spelling),
                                     &read[index], sourceOrder++});
    }
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
      if (actual.orth == nullptr || actual.meaning == nullptr ||
          actual.pos == nullptr || actual.inflection == nullptr) {
        check(false, "every rendered field resolves into the string pool");
        return;
      }
      const render::Rendered rendered =
          render::render(*expected[index].reading);
      const std::array<std::string_view, 4> wanted{
          rendered.orth, rendered.meaning, rendered.pos, rendered.inflection};
      const std::array<std::string_view, 4> found{
          actual.orth, actual.meaning, actual.pos, actual.inflection};
      if (wanted != found) {
        check(false, "every ordered row retains its four rendered fields");
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

} // namespace

void wl_stable::emitter::validatePublishedRelationshipImage(
    const expand::Forms& forms, const expand::Readings& readings,
    const expand::Swept& swept, const std::filesystem::path& path) {
  failures = 0;
  const std::vector<std::byte> source = readImage(path);
  check(!source.empty(), "the generated image has bytes");
  if (failures != 0 || source.empty())
    error::fatal("relationship post validation could not read the image");

  rel::Image accepted;
  std::string failure;
  check(accepted.loadBytes(source, failure), "the generated image is accepted");
  if (!accepted.valid()) {
    std::cerr << "FAIL: " << failure << '\n';
    error::fatal("relationship post validation rejected the image");
  }
  check(failure.empty(), "an accepted image reports no failure");
  check(accepted.bytes() == source.size(), "the image keeps every byte");
  check(accepted.maximumWordLength() <= rel::kMaximumWordLength,
        "the header's spelling length is within the format's capacity");

  const std::vector<std::string> spellings = accepted.spellings();
  check(spellings.size() == accepted.spellingCount(),
        "the walk finds every spelling the header counts");
  validateSemanticRows(accepted, forms, readings, swept, spellings);
  check(accepted.lookup("").count == 0, "the empty word matches nothing");
  check(accepted.lookup("qqqqqqqqqqqqqqqqqqqqqqqqqqqq").count == 0,
        "a word longer than the header allows matches nothing");

  {
    auto b = source;
    b[0] ^= std::byte{1};
    reject(std::move(b), "a wrong magic is refused");
  }
  {
    auto b = source;
    write32(b, 8, rel::kVersion + 1);
    reject(std::move(b), "a wrong version is refused");
  }
  {
    auto b = source;
    write32(b, 12, 44);
    reject(std::move(b), "a wrong header size is refused");
  }
  {
    auto b = source;
    write32(b, 24, rel::kSectionCount - 1);
    reject(std::move(b), "a wrong section count is refused");
  }

  {
    auto b = source;
    write32(b, 36, rel::kMaximumWordLength + 1);
    reject(std::move(b), "a spelling length past capacity is refused");
  }
  {
    auto b = source;
    write32(b, 36, accepted.maximumWordLength() - 1);
    reject(std::move(b), "a spelling length the image exceeds is refused");
  }

  {
    auto b = source;
    b[rel::kHeaderBytes] ^= std::byte{1};
    reject(std::move(b), "a moved section is refused");
  }

  const std::size_t states = accepted.section(rel::Section::States).offset;
  {
    auto b = source;
    b[states + 9] = std::byte{1};
    reject(std::move(b), "a used reserved state byte is refused");
  }

  const std::size_t transitions =
      accepted.section(rel::Section::Transitions).offset;
  {
    auto b = source;
    write32(b, transitions, 0xffffffffu);
    reject(std::move(b), "a cyclic transition is refused");
  }

  const std::size_t instructions =
      accepted.section(rel::Section::Instructions).offset;
  {
    auto b = source;
    b[instructions] &= std::byte{rel::kRelationshipMask};
    reject(std::move(b), "a program that selects no lexeme is refused");
  }

  {
    auto b = source;
    b.back() = std::byte{'x'};
    reject(std::move(b), "an unterminated string pool is refused");
  }

  if (failures != 0) {
    std::cerr << failures << " failure(s)\n";
    error::fatal("relationship post validation failed");
  }
  std::cout
      << "  exhaustive spelling / ordered-row / four-field check: exact\n";
}
