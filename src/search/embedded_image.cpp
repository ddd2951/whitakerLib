// NOTE: The string pool is embedded as char so its pointers are constant
#include "image.hpp"

#include "image_layout.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ranges>

#if defined(__clang__)
#define WHITAKER_EMBED_OFFSET clang::offset
#pragma clang diagnostic ignored "-Wc23-extensions"
#else
#define WHITAKER_EMBED_OFFSET gnu::offset
#pragma GCC diagnostic ignored "-Wc++26-extensions"
#endif

namespace whitaker::relationship {

// NOTE: extern, named outside this file, so each section's start is a link-time constant (image.hpp).
extern constexpr unsigned char kImageBefore[WHITAKER_IMAGE_STRINGS_AT] = {
#embed WHITAKER_IMAGE_FILE limit(WHITAKER_IMAGE_STRINGS_AT)
};
extern constexpr char kImageStrings[WHITAKER_IMAGE_STRINGS_BYTES] = {
#embed WHITAKER_IMAGE_FILE WHITAKER_EMBED_OFFSET(WHITAKER_IMAGE_STRINGS_AT) limit(WHITAKER_IMAGE_STRINGS_BYTES)
};
extern constexpr unsigned char kImageAfter[] = {
#embed WHITAKER_IMAGE_FILE WHITAKER_EMBED_OFFSET(WHITAKER_IMAGE_STRINGS_AT + WHITAKER_IMAGE_STRINGS_BYTES)
};

namespace {

constexpr const auto& kBefore = kImageBefore;
constexpr const auto& kStrings = kImageStrings;
constexpr const auto& kAfter = kImageAfter;
constexpr std::uint64_t kAfterAt = sizeof kBefore + sizeof kStrings;

constexpr Image kFrame = frameOf(kBefore);
static_assert(frameFault(kFrame, sizeof kBefore + sizeof kStrings + sizeof kAfter).empty(),
              "data/whitaker.dat has a bad header or directory");
static_assert(sectionOf(kFrame, Section::Strings).offset == sizeof kBefore &&
                  sectionOf(kFrame, Section::Strings).bytes == sizeof kStrings,
              "image_layout.h is not data/whitaker.dat's string pool");

constexpr Image kFramed = [] {
  Image image = kFrame;
  image.strings = kStrings;
  for (std::uint32_t i = 0; i < kSectionCount; ++i) {
    const std::uint64_t offset = sectionOf(image, Section{i}).offset;
    if (Section{i} != Section::Strings)
      image.records[i] = offset < kAfterAt ? kBefore + offset : kAfter + (offset - kAfterAt);
  }
  return image;
}();

constexpr auto kAddons = [] {
  std::array<Addon, sectionOf(kFramed, Section::Addons).count> addons{};
  for (std::size_t i = 0; i < addons.size(); ++i)
    addons[i] = decodeAddon(kFramed, static_cast<std::uint16_t>(i));
  return addons;
}();
static_assert(std::ranges::is_sorted(kAddons, {}, &Addon::kind), "addonsOf needs the addons grouped by kind");
static_assert(std::ranges::all_of(std::views::iota(std::size_t{0}, kAddons.size()),
                                  [](const std::size_t i) { return kAddons[i].id == i; }),
              "affix marks need an add-on's id to be its place");
} // namespace

extern constexpr std::array<std::uint64_t, 256> kTackonsEndingIn = [] {
  std::array<std::uint64_t, 256> ending{};
  std::size_t i = 0;
  for (const Addon& addon : kAddons)
    if (addon.kind == latin::AddonKind::tackon) {
      // NOTE: A tackon mask is one 64-bit word.
      if (i >= 64)
        std::abort();
      if (!addon.fix.empty())
        ending[static_cast<unsigned char>(addon.fix.back())] |= std::uint64_t{1} << i;
      ++i;
    }
  return ending;
}();

namespace {

static_assert(
    [] {
      for (const Section s : {Section::Back, Section::AffixBack, Section::PrefixFront, Section::SuffixBack})
        for (std::uint64_t i = 0; i < sectionOf(kFramed, s).count; ++i)
          if (readWord(recordAt(kFramed, s, i), kNodeBytes) & facts::kFoldedAwayMask)
            return false;
      return true;
    }(),
    "a node uses a folded-away letter's bit, which two_ended's decode no longer reads as count");
constexpr bool fitsSteps(const Addon& addon) {
  return addon.kind == latin::AddonKind::prefix || addon.kind == latin::AddonKind::suffix || addon.fix.size() >= 2;
}
static_assert(std::ranges::all_of(kAddons, fitsSteps), "facts::kMaxAddonSteps needs other fixes two letters long");

constexpr auto kGrammars = [] {
  std::array<latin::Analysis, sectionOf(kFramed, Section::Grammars).count> grammars{};
  for (std::size_t i = 0; i < grammars.size(); ++i)
    grammars[i] = decodeGrammar(kFramed, static_cast<std::uint16_t>(i));
  return grammars;
}();

constexpr auto kPackedGrammars = [] {
  std::array<PackedGrammar, kGrammars.size()> rows{};
  for (std::size_t i = 0; i < rows.size(); ++i) {
    rows[i].rex = static_cast<std::uint8_t>(std::to_underlying(kGrammars[i].part) * 2);
    if (!latin::packed::packGrammar(kGrammars[i], rows[i].g))
      std::abort();
  }
  std::array<bool, kGrammars.size()> seen{};
  for (std::uint32_t i = 0; i < sectionOf(kFramed, Section::InflectionLabels).count; ++i) {
    const auto label = readRecord<InflectionLabelRecord>(recordAt(kFramed, Section::InflectionLabels, i));
    auto& row = rows[label.description];
    if (seen[label.description])
      row.labelsVary = row.labelsVary || row.age != label.age || row.frequency != label.frequency;
    row.age = label.age;
    row.frequency = label.frequency;
    seen[label.description] = true;
  }
  return rows;
}();

static_assert(std::ranges::equal(
                  kFramed.directory, kDirectory, {}, [](const DirectoryEntry& e) { return e.fields(); },
                  [](const DirectoryEntry& e) { return e.fields(); }),
              "image_layout.h is not data/whitaker.dat's directory");

} // namespace

extern constexpr std::array<PackedGrammar, kPackedGrammars.size()> kImagePackedGrammars = kPackedGrammars;

namespace {

constexpr Image kEmbedded = [] {
  Image image = kFramed;
  image.packedGrammars = kImagePackedGrammars;
  for (const auto kind :
       kAddons | std::views::chunk_by([](const Addon& a, const Addon& b) { return a.kind == b.kind; }))
    image.addonsByKind[std::to_underlying(kind.front().kind)] = {kind};
  return image;
}();

} // namespace

constinit const Image* sImage = &kEmbedded;

} // namespace whitaker::relationship
