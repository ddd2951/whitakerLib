#include "image_writer.hpp"

#include "emitter/image_file.hpp"
#include "error/error.hpp"
#include "util/reflect_util.hpp"
#include "src/search/relationship_schema.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace rel = whitaker::relationship;

namespace emitter {

namespace {

template <typename T> void append(std::vector<unsigned char>& out, const T& value) {
  if constexpr (requires { value.fields(); }) {
    std::array<unsigned char, rel::recordBytes<T>()> bytes{};
    rel::storeRecord(bytes.data(), value);
    out.insert(out.end(), bytes.begin(), bytes.end());
  } else {
    std::array<unsigned char, sizeof value> bytes{};
    rel::storeField(bytes.data(), value);
    out.insert(out.end(), bytes.begin(), bytes.end());
  }
}

void appendWord(std::vector<unsigned char>& out, std::uint64_t word, std::size_t bytes) {
  for (std::size_t i = 0; i < bytes; ++i)
    out.push_back(static_cast<unsigned char>(word >> (8 * i)));
}

} // namespace

[[nodiscard]] ImageFile encodeImage(const ImageData& imageData, const Machine& machine) {
  ImageFile image;
  image.bytes.resize(rel::kHeaderBytes + rel::kSectionCount * rel::kDirectoryBytes);
  image.parts = {{"header", 0, rel::kHeaderBytes, 1, rel::kHeaderBytes},
                 {"directory", rel::kHeaderBytes, image.bytes.size(), rel::kSectionCount, rel::kDirectoryBytes}};
  std::array<rel::DirectoryEntry, rel::kSectionCount> entries{};

  // NOTE: `count` is the section's items; the instructions and the strings give their directory another count.
  const auto section = [&](rel::Section id, std::size_t count, const auto& write,
                           std::optional<std::uint64_t> directoryCount = {}) {
    const std::size_t start = image.bytes.size();
    write(image.bytes);
    const std::size_t width = rel::widthOf(id);
    if (image.bytes.size() - start != count * width)
      error::fatal("image: a section's bytes are not its records");
    entries[static_cast<std::size_t>(id)] = {start, image.bytes.size() - start, directoryCount.value_or(count)};
    image.parts.push_back({util::enumToSv(id), start, image.bytes.size(), count, width});
  };
  const auto all = [](const auto& items) {
    return [&items](std::vector<unsigned char>& out) {
      for (const auto& item : items)
        append(out, item);
    };
  };

  section(rel::Section::States, machine.states.size(), all(machine.states));
  section(rel::Section::Transitions, machine.transitions.size(), all(machine.transitions));
  section(rel::Section::Lexemes, imageData.program.lexemes.size(), all(imageData.program.lexemes));
  section(rel::Section::Paradigms, imageData.program.paradigms.size(), all(imageData.program.paradigms));
  section(rel::Section::Targets, imageData.program.targets.size(), all(imageData.program.targets));
  section(
      rel::Section::Instructions, imageData.program.instructions.size(),
      [&](std::vector<unsigned char>& out) {
        for (const std::byte instruction : imageData.program.instructions)
          out.push_back(std::to_integer<unsigned char>(instruction));
      },
      imageData.analyses.size());
  section(rel::Section::Dictionaries, imageData.dictionaries.size(), all(imageData.dictionaries));
  section(rel::Section::Addons, imageData.addons.size(), all(imageData.addons));
  section(rel::Section::Classes, imageData.classes.size(), all(imageData.classes));
  section(rel::Section::FallbackRows, imageData.fallbackRows.size(), all(imageData.fallbackRows));
  section(rel::Section::Inflections, imageData.inflections.size(), all(imageData.inflections));
  section(rel::Section::Endings, imageData.endings.size(), all(imageData.endings));
  section(rel::Section::FallbackStems, imageData.fallbackStems.size(), all(imageData.fallbackStems));
  section(
      rel::Section::Strings, imageData.strings.blob().size(),
      [&](std::vector<unsigned char>& out) {
        out.insert(out.end(), imageData.strings.blob().begin(), imageData.strings.blob().end());
      },
      imageData.strings.count());
  section(rel::Section::Grammars, imageData.grammars.size(), [&](std::vector<unsigned char>& out) {
    for (const latin::Analysis& grammar : imageData.grammars)
      appendWord(out, rel::packGrammar(grammar), rel::kGrammarBytes);
  });
  section(rel::Section::Entries, imageData.entries.size(), [&](std::vector<unsigned char>& out) {
    for (const rel::Entry& entry : imageData.entries)
      appendWord(out, rel::packEntry(entry), rel::kEntryBytes);
  });
  section(rel::Section::StemPatterns, imageData.stemPatterns.size(), all(imageData.stemPatterns));

  rel::storeRecord(image.bytes.data(),
                   rel::HeaderRecord{.magic = rel::kMagic,
                                     .version = rel::kVersion,
                                     .headerBytes = std::uint32_t{rel::kHeaderBytes},
                                     .imageBytes = image.bytes.size(),
                                     .sectionCount = rel::kSectionCount,
                                     .root = machine.root,
                                     .rootOutput = machine.rootOutput,
                                     .maximumWordLength = facts::kMaxWordCharacters,
                                     .spellingCount = static_cast<std::uint32_t>(imageData.words.size()),
                                     .resultCount = static_cast<std::uint32_t>(imageData.analyses.size())});
  for (std::size_t i = 0; i < entries.size(); ++i)
    rel::storeRecord(image.bytes.data() + rel::kHeaderBytes + i * rel::kDirectoryBytes, entries[i]);
  return image;
}

} // namespace emitter
