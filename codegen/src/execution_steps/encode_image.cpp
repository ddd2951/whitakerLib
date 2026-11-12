#include "emitter/relationship_emitter/image_writer.hpp"

#include "execution_steps/steps.hpp"
#include "storage/encoded_image.hpp"
#include "storage/image.hpp"

#include <print>

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

void appendWord(std::vector<unsigned char>& out, const std::uint64_t word, const std::size_t bytes) {
  for (std::size_t i = 0; i < bytes; ++i)
    out.push_back(static_cast<unsigned char>(word >> (8 * i)));
}

} // namespace

[[nodiscard]] ImageFile encodeImage(const ImageData& imageData) {
  ImageFile image{};
  image.bytes.resize(rel::kHeaderBytes + rel::kSectionCount * rel::kDirectoryBytes);
  image.parts = {{"header", 0, rel::kHeaderBytes, 1, rel::kHeaderBytes},
                 {"directory", rel::kHeaderBytes, image.bytes.size(), rel::kSectionCount, rel::kDirectoryBytes}};
  std::array<rel::DirectoryEntry, rel::kSectionCount> entries{};

  // NOTE: `count` is the section's items; the instructions and the strings give their directory another count.
  const auto section = [&](const rel::Section id, const std::size_t count, const auto& write,
                           const std::optional<std::uint64_t> directoryCount = {}) {
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

  section(rel::Section::Dictionaries, imageData.dictionaries.size(), all(imageData.dictionaries));
  section(rel::Section::Addons, imageData.addons.size(), all(imageData.addons));
  section(rel::Section::Classes, imageData.classes.size(), all(imageData.classes));
  section(rel::Section::ClassRows, imageData.classRows.size(), all(imageData.classRows));
  section(rel::Section::Inflections, imageData.inflections.size(), all(imageData.inflections));
  section(rel::Section::Endings, imageData.endings.size(), all(imageData.endings));
  section(rel::Section::Stems, imageData.stems.size(), all(imageData.stems));
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
  section(rel::Section::InflectionLabels, imageData.inflectionLabels.size(), all(imageData.inflectionLabels));
  section(rel::Section::TrimmedReadings, imageData.trimmedReadings.size(), all(imageData.trimmedReadings));

  const auto& t = imageData.twoEnded;
  const auto nodes = [](const auto& items) {
    return [&items](std::vector<unsigned char>& out) {
      for (const auto& n : items) {
        if (n.mask & facts::kFoldedAwayMask)
          error::fatal("image: source node uses a folded-away letter");
        auto bits = std::uint64_t{n.mask} | (std::uint64_t{n.first} << rel::kNodeMaskBits) |
                    (std::uint64_t{n.run.first} << (rel::kNodeMaskBits + rel::kNodeFirstBits)) |
                    ((std::uint64_t{n.run.count} & ((std::uint64_t{1} << rel::kNodeCountBits) - 1))
                     << (rel::kNodeMaskBits + rel::kNodeFirstBits + rel::kNodeRunBits));
        auto payload = n.run.count >> rel::kNodeCountBits;
        for (auto free = facts::kFoldedAwayMask; free; free &= free - 1) {
          bits |= std::uint64_t{payload & 1u} << std::countr_zero(free);
          payload >>= 1;
        }
        if (payload)
          error::fatal("image: source node count exceeds the generated payload");
        appendWord(out, bits, rel::kNodeBytes);
      }
    };
  };
  section(rel::Section::Front, t.sharedFront.size(), all(t.sharedFront));
  section(rel::Section::Back, t.back.size(), nodes(t.back));
  section(rel::Section::Rules, t.rules.size(), all(t.rules));
  section(rel::Section::Cells, t.cells.size(), all(t.cells));
  section(rel::Section::CellIndex, t.cellIndex.size(), all(t.cellIndex));
  section(rel::Section::CellOf, t.cellOf.size(), all(t.cellOf));
  section(rel::Section::RowIds, t.rowIds.size(), all(t.rowIds));
  section(rel::Section::AffixBack, t.affixBack.size(), nodes(t.affixBack));
  section(rel::Section::PrefixFront, t.prefixFront.size(), nodes(t.prefixFront));
  section(rel::Section::SuffixBack, t.suffixBack.size(), nodes(t.suffixBack));
  section(rel::Section::AddonIds, t.addonIds.size(), all(t.addonIds));
  section(rel::Section::FrontRuns, t.frontRuns.size(), all(t.frontRuns));

  rel::storeRecord(image.bytes.data(),
                   rel::HeaderRecord{.magic = rel::kMagic,
                                     .version = rel::kVersion,
                                     .headerBytes = std::uint32_t{rel::kHeaderBytes},
                                     .imageBytes = image.bytes.size(),
                                     .sectionCount = rel::kSectionCount,
                                     .maximumWordLength = facts::kMaxWordCharacters,
                                     .spellingCount = static_cast<std::uint32_t>(imageData.words.size()),
                                     .resultCount = static_cast<std::uint32_t>(imageData.analyses.size())});
  for (std::size_t i = 0; i < entries.size(); ++i)
    rel::storeRecord(image.bytes.data() + rel::kHeaderBytes + i * rel::kDirectoryBytes, entries[i]);
  return image;
}

} // namespace emitter

// TODO: After 0.4.0, reconsider splitting image encoding into header, section and directory steps.
void execution_steps::encoding::encodeImage() {
  const auto& imageData = storage::image;
  emitter::ImageFile image = emitter::encodeImage(imageData);
  emitter::printReport(image);
  std::println("  {} spellings, {} ordered analyses", imageData.words.size(), imageData.analyses.size());
  storage::encodedImage = std::move(image);
}
