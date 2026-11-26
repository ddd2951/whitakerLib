#include <whitaker.hpp>
#include "image_checksum.h"
#include "parse/adverb.hpp"
#include "parse/find_readings.hpp"
#include "parse/respellings.hpp"
#include "parse/result.hpp"
#include "per_thread.hpp"

#include <cstdlib>
#include <cstring>
#include <new>
#include <limits>
#include <type_traits>

namespace whitaker {
namespace {
using namespace parse;
[[nodiscard, gnu::always_inline]] inline Work& work() { return PerThread<Work>::get(); }

[[nodiscard]] bool looksLikeAName(const std::string_view word) {
  return word.size() > 1 && word[0] >= 'A' && word[0] <= 'Z' && word[1] >= 'a' && word[1] <= 'z';
}
[[nodiscard]] bool readWord(const std::string_view word) {
  work().count = 0;
  work().changeCount = 0;
  work().trimmed = 0;
  work().outOfMemory = false;
  work().word = word;
  WHITAKER_TRACE_HOOK(begin());
  if (word.empty())
    return true;
  findReadings(work(), word);
  if (!foundOrTrimmed(work()) && !looksLikeAName(word) && !addRespelledReadings(work(), word))
    addRespelledEncliticReadings(work(), word);
  WHITAKER_TRACE_HOOK(respelled(work()));
  addAdverbsOfAdjectives(work());
  WHITAKER_TRACE_HOOK(ran(whitaker::trace::Pass::adverbs, work()));
  WHITAKER_TRACE_HOOK(end(work()));
  return !work().outOfMemory;
}

template <typename T> [[nodiscard]] bool reserve(T*& items, std::size_t& capacity, const std::size_t needed) {
  if (needed <= capacity)
    return true;
  constexpr auto maximum = std::numeric_limits<std::size_t>::max() / sizeof(T);
  if (needed > maximum)
    return false;
  std::size_t grown = capacity ? capacity : std::min(std::size_t{16}, maximum);
  while (grown < needed)
    grown = grown > maximum / 2 ? maximum : grown * 2;
  auto* const moved = static_cast<T*>(std::realloc(items, grown * sizeof(T)));
  if (!moved)
    return false;
  items = moved;
  capacity = grown;
  return true;
}
} // namespace

struct Result::Storage {
  std::size_t wordCapacity{}, readingCapacity{}, dataCapacity{}, dataBytes{};
};
static_assert(std::is_trivially_copyable_v<Result::Storage>);

namespace {

[[nodiscard]] bool appendWord(Result& result, const char* const spelling, const std::size_t size) {
  auto& s = *result.storage;
  const auto count = static_cast<std::size_t>(work().count);
  constexpr auto maximum = std::numeric_limits<std::size_t>::max();
  if (result.wordCount == maximum / sizeof(WordOffsets) || count > maximum / sizeof(Reading) - result.readingCount)
    return false;
  unsigned char remap[256]{};
  unsigned used = 0;
  std::size_t bytes = 0;
  unsigned char encoded[sizeof(Change)];
  for (int i = 0; i < work().count; ++i) {
    const unsigned n = work().matches[i].changes;
    if (n && !remap[n]) {
      remap[n] = static_cast<unsigned char>(++used);
      bytes += encodeChange(work().changes[n - 1], encoded);
    }
  }
  bytes += used * 2 + size + 2;
  if (bytes > maximum - s.dataBytes || !reserve(result.words, s.wordCapacity, result.wordCount + 1) ||
      !reserve(result.readings, s.readingCapacity, result.readingCount + count) ||
      !reserve(result.data, s.dataCapacity, s.dataBytes + bytes))
    return false;
  auto* const data = result.data + s.dataBytes;
  data[0] = static_cast<unsigned char>(size | (work().trimmed ? boundary::kTrimmedFlag : 0u));
  if (size)
    std::memcpy(data + 1, spelling, size);
  data[size + 1] = 0;
  auto* const pool = data + size + 2;
  unsigned offset = used * 2;
  for (unsigned n = 1; n <= work().changeCount; ++n)
    if (remap[n]) {
      const unsigned at = unsigned(remap[n] - 1) * 2;
      pool[at] = static_cast<unsigned char>(offset);
      pool[at + 1] = static_cast<unsigned char>(offset >> 8);
      offset += encodeChange(work().changes[n - 1], pool + offset);
    }
  for (std::size_t i = 0; i < count; ++i) {
    const auto& r = work().matches[i];
    result.readings[result.readingCount + i] = r;
    result.readings[result.readingCount + i].changes = remap[r.changes];
  }
  result.words[result.wordCount++] = {.firstReading = result.readingCount, .firstData = s.dataBytes};
  result.readingCount += count;
  s.dataBytes += bytes;
  return true;
}

} // namespace

Status boundary::read(Result& result, const char* const word, const std::size_t size, const bool replace) noexcept {
  if (!word && size)
    return Status::invalidArgument;
  if (size > kMaxWordLength)
    return Status::inputTooLong;
  char copy[kMaxWordLength];
  if (size)
    std::memcpy(copy, word, size);
  if (replace)
    clear(result);
  if (!result.storage) {
    result.storage = new (std::nothrow) Result::Storage{};
    if (!result.storage)
      return Status::outOfMemory;
  }
  return readWord({copy, size}) && appendWord(result, copy, size) ? Status::ok : Status::outOfMemory;
}

bool boundary::cloneInto(const Result& result, Result& copy) noexcept {
  if (!result.storage || !result.wordCount)
    return true;
  copy.storage = new (std::nothrow) Result::Storage{};
  if (!copy.storage)
    return false;
  auto& s = *copy.storage;
  if (!reserve(copy.words, s.wordCapacity, result.wordCount) ||
      !reserve(copy.readings, s.readingCapacity, result.readingCount) ||
      !reserve(copy.data, s.dataCapacity, result.storage->dataBytes))
    return false;
  std::memcpy(copy.words, result.words, result.wordCount * sizeof(WordOffsets));
  if (result.readingCount)
    std::memcpy(copy.readings, result.readings, result.readingCount * sizeof(Reading));
  std::memcpy(copy.data, result.data, result.storage->dataBytes);
  copy.wordCount = result.wordCount;
  copy.readingCount = result.readingCount;
  s.dataBytes = result.storage->dataBytes;
  return true;
}

Result::~Result() {
  std::free(words);
  std::free(readings);
  std::free(data);
  delete storage;
}

void clear(Result& result) noexcept {
  result.wordCount = result.readingCount = 0;
  if (result.storage)
    result.storage->dataBytes = 0;
}

ReadingDetail detail(const Result& result, const WordId word, const std::size_t readingIndex) noexcept {
  ReadingDetail out{};
  const auto all = readings(result, word);
  if (readingIndex >= all.size())
    return out;
  const Reading& r = all[readingIndex];
  const auto text = spelling(result, word);
  Change c{};
  if (r.changes) {
    const auto* const pool = result.data + result.words[std::to_underlying(word)].firstData + text.size() + 2;
    const unsigned at = unsigned(r.changes - 1) * 2;
    c = decodeChange(pool + (pool[at] | unsigned(pool[at + 1]) << 8));
  }
  parse::decode(text, r, c, out);
  return out;
}

unsigned imageFormat() noexcept { return rel::kVersion; }
const char* imageChecksum() noexcept { return WHITAKER_IMAGE_SHA256; }
const char* addonSpelling(const std::uint16_t id) noexcept { return rel::addon(id).fix.data(); }
const char* addonMeaning(const std::uint16_t id) noexcept { return rel::addon(id).meaning; }
DictionaryWordStems stems(const DictionaryWordId id) noexcept { return rel::stems(std::to_underlying(id)); }
DictionaryForm dictionaryForm(const Result& result, const WordId word, const std::size_t readingIndex) noexcept {
  const auto all = readings(result, word);
  if (readingIndex >= all.size())
    return {};
  const auto& r = all[readingIndex];
  const auto governedCase = r.part == latin::Part::preposition ? parse::grammar(r).caseOf : latin::Case{};
  return rel::dictionaryForm(r.entry, governedCase);
}
SpellingRule spellingRule(const std::uint8_t rule) noexcept { return parse::spellingRule(rule); }
} // namespace whitaker
