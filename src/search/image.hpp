#pragma once

#include "relationship_image.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <utility>
#include <type_traits>

namespace whitaker::relationship {

struct PackedGrammar {
  std::uint8_t rex{}, g[3]{};
  latin::Age age{};
  latin::Frequency frequency{};
  bool labelsVary{};
};
static_assert(std::is_trivially_copyable_v<PackedGrammar>);

struct Image {
  std::array<const unsigned char*, kSectionCount> records{};
  const char* strings{};
  HeaderRecord header{};
  std::array<DirectoryEntry, kSectionCount> directory{};
  std::array<std::span<const Addon>, std::to_underlying(latin::AddonKind::packon) + 1> addonsByKind{};
  std::span<const latin::Analysis> grammars{};
  std::span<const PackedGrammar> packedGrammars{};
};
static_assert(std::is_trivially_copyable_v<Image>);

extern const Image* sImage;

[[nodiscard]] constexpr const unsigned char* recordAt(const Image& image, Section section, std::uint64_t index);
[[nodiscard]] constexpr const DirectoryEntry& sectionOf(const Image& image, Section section);

#ifdef WHITAKER_EMBEDDED_FRAME
// INFO: The embedded image's place is fixed at link time, so a section's start and count are constants, not loads.
} // namespace whitaker::relationship
#include "image_layout.h"
namespace whitaker::relationship {
inline constexpr std::array<DirectoryEntry, kSectionCount> kDirectory{{WHITAKER_IMAGE_DIRECTORY}};
inline constexpr std::uint64_t kImageAfterAt = WHITAKER_IMAGE_STRINGS_AT + WHITAKER_IMAGE_STRINGS_BYTES;
[[gnu::visibility("hidden")]] extern const unsigned char kImageBefore[WHITAKER_IMAGE_STRINGS_AT];
[[gnu::visibility("hidden")]] extern const char kImageStrings[WHITAKER_IMAGE_STRINGS_BYTES];
[[gnu::visibility("hidden")]] extern const unsigned char kImageAfter[];
[[gnu::visibility(
    "hidden")]] extern const std::array<PackedGrammar, kDirectory[std::to_underlying(Section::Grammars)].count>
    kImagePackedGrammars;

[[nodiscard, gnu::always_inline]] inline const unsigned char* imageRecord(const Section section,
                                                                          const std::uint64_t index) {
  const std::uint64_t offset = kDirectory[std::to_underlying(section)].offset;
  return (offset < WHITAKER_IMAGE_STRINGS_AT ? kImageBefore + offset : kImageAfter + (offset - kImageAfterAt)) +
         index * widthOf(section);
}
[[nodiscard, gnu::always_inline]] constexpr const DirectoryEntry& frameSection(const Section section) {
  return kDirectory[std::to_underlying(section)];
}
[[nodiscard, gnu::always_inline]] inline const char* imageStrings() { return kImageStrings; }
[[nodiscard, gnu::always_inline]] inline std::span<const PackedGrammar> packedGrammars() {
  return kImagePackedGrammars;
}
#else
[[nodiscard]] inline const unsigned char* imageRecord(const Section section, const std::uint64_t index) {
  return recordAt(*sImage, section, index);
}
[[nodiscard]] inline const DirectoryEntry& frameSection(const Section section) { return sectionOf(*sImage, section); }
[[nodiscard]] inline const char* imageStrings() { return sImage->strings; }
[[nodiscard]] inline std::span<const PackedGrammar> packedGrammars() { return sImage->packedGrammars; }
#endif

// NOTE: Checks nothing.
[[nodiscard]] constexpr Image frameOf(const unsigned char* const frame) {
  Image image{};
  image.header = readRecord<HeaderRecord>(frame);
  for (std::size_t i = 0; i < kSectionCount; ++i)
    image.directory[i] = readRecord<DirectoryEntry>(frame + kHeaderBytes + i * kDirectoryBytes);
  return image;
}

[[nodiscard]] constexpr const DirectoryEntry& sectionOf(const Image& image, const Section section) {
  return image.directory[static_cast<std::size_t>(section)];
}

[[nodiscard]] constexpr const unsigned char* recordAt(const Image& image, const Section section,
                                                      const std::uint64_t index) {
  return image.records[static_cast<std::size_t>(section)] + index * widthOf(section);
}

[[nodiscard]] constexpr std::string_view frameFault(const Image& image, const std::uint64_t size) {
  const HeaderRecord& header = image.header;
  if (header.magic != kMagic || header.version != kVersion || header.headerBytes != kHeaderBytes ||
      header.imageBytes != size || header.sectionCount != kSectionCount)
    return "Invalid: image header";
  if (header.maximumWordLength == 0 || header.maximumWordLength > facts::kMaxWordCharacters ||
      header.spellingCount == 0 || header.resultCount == 0)
    return "Invalid: image header counts";

  std::uint64_t cursor = kHeaderBytes + kSectionCount * kDirectoryBytes;
  for (const DirectoryEntry& entry : image.directory) {
    if (entry.offset != cursor || entry.offset > size || entry.bytes > size - entry.offset)
      return "Invalid: image section order or bounds";
    cursor += entry.bytes;
  }
  if (cursor != size)
    return "Invalid: image section coverage";

  for (std::uint32_t i = 0; i < kSectionCount; ++i) {
    const Section id{i};
    const DirectoryEntry& entry = sectionOf(image, id);
    const std::uint64_t width = widthOf(id);
    if (id != Section::Strings &&
        (entry.count > std::numeric_limits<std::uint64_t>::max() / width || entry.bytes != entry.count * width))
      return "Invalid: image section width";
  }
  if (sectionOf(image, Section::Entries).count != sectionOf(image, Section::Dictionaries).count ||
      sectionOf(image, Section::DictionaryStemRuns).count != sectionOf(image, Section::Dictionaries).count + 1 ||
      sectionOf(image, Section::DictionaryStems).count != sectionOf(image, Section::Stems).count ||
      sectionOf(image, Section::StemLetters).count != sectionOf(image, Section::Stems).count ||
      sectionOf(image, Section::Addons).count > std::numeric_limits<std::uint16_t>::max())
    return "Invalid: image section counts";
  return {};
}

[[nodiscard]] constexpr Addon decodeAddon(const Image& image, const std::uint16_t index) {
  const AddonRecord record = readRecord<AddonRecord>(recordAt(image, Section::Addons, index));
  return {
      .id = index,
      .fix = image.strings + record.fix,
      .meaning = image.strings + record.meaning,
      .target = record.target,
      .kind = record.kind,
      .root = record.root,
      .targetPart = record.targetPart,
      .rootStemKey = record.rootStemKey,
      .targetStemKey = record.targetStemKey,
      .connectingLetter = static_cast<char>(record.connectingLetter),
      .targetRules = record.targetRules,
      .firstLetterAsSpelled = static_cast<char>(record.firstLetterAsSpelled),
      .rows = rows(record),
  };
}

[[nodiscard]] constexpr latin::Analysis decodeGrammar(const Image& image, const std::uint16_t index) {
  return unpackGrammar(readWord(recordAt(image, Section::Grammars, index), kGrammarBytes)).analysis;
}

} // namespace whitaker::relationship
