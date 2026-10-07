#include "image.hpp"

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

bool loadImage(std::span<const std::byte> bytes, std::string& failure) {
  assert(bytes.size() >= kHeaderBytes + kSectionCount * kDirectoryBytes);
  const auto* frame = reinterpret_cast<const unsigned char*>(bytes.data());
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

std::vector<std::string> spellings() {
  struct Visit {
    std::uint32_t state;
    std::string text;
  };
  std::vector<std::string> result;
  result.reserve(header().spellingCount);
  std::vector<Visit> pending{{header().root, {}}};
  while (!pending.empty()) {
    Visit visit = std::move(pending.back());
    pending.pop_back();
    const auto state = readRecord<StateRecord>(recordAt(*sImage, Section::States, visit.state));
    if (state.resultCount != 0)
      result.push_back(visit.text);
    std::uint32_t transition = state.firstTransition;
    for (unsigned letter = 0; letter < facts::kLetterCount; ++letter) {
      if ((state.letterMask & (1u << letter)) == 0)
        continue;
      const auto edge = readRecord<TransitionRecord>(recordAt(*sImage, Section::Transitions, transition++));
      pending.push_back(Visit{edge.target, visit.text + static_cast<char>('a' + letter)});
    }
  }
  std::ranges::sort(result);
  return result;
}

} // namespace whitaker::relationship
