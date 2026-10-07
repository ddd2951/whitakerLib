// The string pool is embedded as char so its pointers are constant
#include "image.hpp"

#include "image_layout.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <ranges>

#if defined(__clang__)
#define WHITAKER_EMBED_OFFSET clang::offset
#pragma clang diagnostic ignored "-Wc23-extensions"
#else
#define WHITAKER_EMBED_OFFSET gnu::offset
#pragma GCC diagnostic ignored "-Wc++26-extensions"
#endif

namespace whitaker::relationship {
namespace {

constexpr unsigned char kBefore[] = {
#embed WHITAKER_IMAGE_FILE limit(WHITAKER_IMAGE_STRINGS_AT)
};
constexpr char kStrings[] = {
#embed WHITAKER_IMAGE_FILE WHITAKER_EMBED_OFFSET(WHITAKER_IMAGE_STRINGS_AT) limit(WHITAKER_IMAGE_STRINGS_BYTES)
};
constexpr unsigned char kAfter[] = {
#embed WHITAKER_IMAGE_FILE WHITAKER_EMBED_OFFSET(WHITAKER_IMAGE_STRINGS_AT + WHITAKER_IMAGE_STRINGS_BYTES)
};
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
constexpr bool fitsSteps(const Addon& addon) {
  return addon.kind == latin::AddonKind::Prefix || addon.kind == latin::AddonKind::Suffix || addon.fix.size() >= 2;
}
static_assert(std::ranges::all_of(kAddons, fitsSteps), "WHITAKER_MAX_ADDON_STEPS needs other fixes two letters long");

constexpr auto kGrammars = [] {
  std::array<latin::Analysis, sectionOf(kFramed, Section::Grammars).count> grammars{};
  for (std::size_t i = 0; i < grammars.size(); ++i)
    grammars[i] = decodeGrammar(kFramed, static_cast<std::uint16_t>(i));
  return grammars;
}();

constexpr Image kEmbedded = [] {
  Image image = kFramed;
  image.grammars = kGrammars;
  for (const auto kind :
       kAddons | std::views::chunk_by([](const Addon& a, const Addon& b) { return a.kind == b.kind; }))
    image.addonsByKind[std::to_underlying(kind.front().kind)] = {kind};
  return image;
}();

} // namespace

constinit const Image* sImage = &kEmbedded;

} // namespace whitaker::relationship
