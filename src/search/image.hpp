#pragma once

#include "relationship_image.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string_view>
#include <utility>

namespace whitaker::relationship {

struct Image {
  std::array<const unsigned char*, kSectionCount> records{};
  const char* strings{};
  HeaderRecord header{};
  std::array<DirectoryEntry, kSectionCount> directory{};
  std::array<std::span<const Addon>, std::to_underlying(latin::AddonKind::Packon) + 1> addonsByKind{};
  std::span<const latin::Analysis> grammars{};
};

extern const Image* sImage;

// Checks nothing.
[[nodiscard]] constexpr Image frameOf(const unsigned char* frame) noexcept {
  Image image{};
  image.header = readRecord<HeaderRecord>(frame);
  for (std::size_t i = 0; i < kSectionCount; ++i)
    image.directory[i] = readRecord<DirectoryEntry>(frame + kHeaderBytes + i * kDirectoryBytes);
  return image;
}

[[nodiscard]] constexpr const DirectoryEntry& sectionOf(const Image& image, Section section) noexcept {
  return image.directory[static_cast<std::size_t>(section)];
}

[[nodiscard]] constexpr const unsigned char* recordAt(const Image& image, Section section,
                                                      std::uint64_t index) noexcept {
  return image.records[static_cast<std::size_t>(section)] + index * widthOf(section);
}

[[nodiscard]] constexpr std::string_view frameFault(const Image& image, std::uint64_t size) noexcept {
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
    if (id != Section::Instructions && id != Section::Strings &&
        (entry.count > std::numeric_limits<std::uint64_t>::max() / width || entry.bytes != entry.count * width))
      return "Invalid: image section width";
  }
  if (sectionOf(image, Section::Entries).count != sectionOf(image, Section::Dictionaries).count ||
      sectionOf(image, Section::Addons).count > std::numeric_limits<std::uint16_t>::max() ||
      header.root >= sectionOf(image, Section::States).count ||
      header.rootOutput >= sectionOf(image, Section::Instructions).bytes)
    return "Invalid: image section counts";
  return {};
}

[[nodiscard]] constexpr Addon decodeAddon(const Image& image, std::uint16_t index) noexcept {
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
      .rows = record.rows(),
  };
}

[[nodiscard]] constexpr latin::Analysis decodeGrammar(const Image& image, std::uint16_t index) noexcept {
  return unpackGrammar(readWord(recordAt(image, Section::Grammars, index), kGrammarBytes)).analysis;
}

} // namespace whitaker::relationship
