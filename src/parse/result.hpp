#pragma once

#include <whitaker.h>

#include "facts.hpp"
#include "parse/arena.hpp"
#include "search/addon_readings.hpp"
#include "search/relationship_image.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string_view>

namespace whitaker::parse {

namespace rel = whitaker::relationship;

static_assert(WHITAKER_MAX_WORD_LENGTH == facts::kMaxWordCharacters, "the public word length must be the corpus fact");
inline constexpr std::size_t kRomanMeaningBytes = 64;

// INFO: One working reading, with add-on steps stored inline.
struct Match {
  const char* orth;
  const char* ending;
  const char* meaning;
  const char* pos;
  WhitakerGrammar grammar;
  WhitakerEntry entry;
  WhitakerAddonStep addons[facts::kMaxAddonSteps];
  std::uint8_t addonCount;
  std::uint8_t route;
  std::uint8_t spellingRule;
  std::uint8_t inflectionAge;
  std::uint8_t inflectionFrequency;
  std::uint8_t whyTrimmed;
};

// INFO: A reusable buffer for one word's readings.
struct Work {
  Work() noexcept = default;
  Work(const Work&) = delete;
  Work& operator=(const Work&) = delete;
  ~Work() { std::free(matches); }

  Match* matches = nullptr;
  int count = 0;
  int capacity = 0;
  std::uint8_t trimmed = 0;
  bool outOfMemory = false;
};

class Text {
public:
  using Checkpoint = Arena::Mark;

  explicit Text(Arena& arena) noexcept : m_arena{arena} {}

  [[nodiscard]] Checkpoint checkpoint() const noexcept { return m_arena.mark(); }

  void rewind(Checkpoint checkpoint) noexcept { m_arena.rewind(checkpoint); }

  [[nodiscard]] const char* keep(std::string_view value) noexcept {
    auto* const result = static_cast<char*>(m_arena.allocate(value.size() + 1));
    if (result == nullptr) {
      m_outOfMemory = true;
      return "";
    }
    // NOTE: May overlap after a rewind (keepOnly).
    std::memmove(result, value.data(), value.size());
    result[value.size()] = '\0';
    return result;
  }

  [[nodiscard]] const char* keepFolded(std::string_view value) noexcept {
    char folded[facts::kMaxWordCharacters];
    assert(value.size() <= sizeof folded);
    std::ranges::transform(value, folded, facts::foldLetter);
    return keep({folded, value.size()});
  }

  [[nodiscard]] bool outOfMemory() const noexcept { return m_outOfMemory; }

private:
  Arena& m_arena;
  bool m_outOfMemory = false;
};

[[nodiscard]] WhitakerGrammar grammarOf(const latin::Analysis& a) noexcept;
[[nodiscard]] latin::Analysis analysisOf(const WhitakerGrammar& g) noexcept;
// NOTE: Null when out of memory. Growing may move the readings: take no reference across it.
[[nodiscard]] Match* newMatch(Work& out) noexcept;
[[nodiscard]] inline bool foundOrTrimmed(const Work& out) noexcept { return out.count != 0 || out.trimmed; }
void fillFromImage(Match& match, Text& text, std::string_view spelling, const rel::Result& reading) noexcept;
void addDictionaryReadings(Work& out, Text& text, std::string_view spelling,
                           bool (*accept)(const rel::Result&) = nullptr) noexcept;
void prependAddon(Work& out, int first, const rel::Addon& addon) noexcept;
void addAddonReadings(Work& out, Text& text, std::string_view word, const whitaker::AddonReadings& found) noexcept;

// NOTE: Rewinding discards retained text too; copy it back while compacting readings.
template <typename Keep>
void keepOnly(Work& out, Text& text, Text::Checkpoint checkpoint, int from, Keep&& keep) noexcept {
  text.rewind(checkpoint);
  int write = from;
  for (int i = from; i < out.count; ++i) {
    if (!keep(i))
      continue;
    const std::string_view stem{out.matches[i].orth}, ending{out.matches[i].ending};
    out.matches[write] = out.matches[i];
    out.matches[write].orth = text.keep(stem);
    out.matches[write++].ending = text.keep(ending);
  }
  out.count = write;
}

} // namespace whitaker::parse
