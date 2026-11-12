#include "image.hpp"
#include "two_ended.hpp"

#include "facts.hpp"

#include <algorithm>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

namespace whitaker::relationship {

const Image* sImage = nullptr;

namespace {
Image sLoaded;
} // namespace

bool loadImage(const std::span<const std::byte> bytes, std::string& failure) {
  assert(bytes.size() >= kHeaderBytes + kSectionCount * kDirectoryBytes);
  const auto* const frame = reinterpret_cast<const unsigned char*>(bytes.data());
  Image image = frameOf(frame);
  if (const std::string_view fault = frameFault(image, bytes.size()); !fault.empty()) {
    failure = fault;
    return false;
  }
  for (std::size_t i = 0; i < kSectionCount; ++i)
    image.records[i] = frame + image.directory[i].offset;
  image.strings = reinterpret_cast<const char*>(frame) + sectionOf(image, Section::Strings).offset;
  sLoaded = image;
  sImage = &sLoaded;
  return true;
}

std::vector<std::string> spellings() { return two_ended::spellings(false); }

} // namespace whitaker::relationship
