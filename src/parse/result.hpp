#pragma once

#include <whitaker.hpp>

#include "facts.hpp"
#include "parse/change_capacity.hpp"
#include "search/addon_readings.hpp"
#include "search/relationship_image.hpp"
#include "trace.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string_view>
#include <type_traits>

namespace whitaker::parse {
namespace rel = whitaker::relationship;

// INFO: An add-on id that names none.
inline constexpr std::uint16_t kNoAddon = 0xFFFF;
static_assert(kMaxWordLength == facts::kMaxWordCharacters);

enum class CRoute : std::uint8_t {
  direct,
  syncopeIi,
  syncopeS,
  syncopeR,
  syncopeIer,
  syncopeIs,
  adverb,
  slur,
  wordMod,
};

// INFO: Only exceptional values. The image supplies the entry, column, stem pattern and ordinary stem length.
struct Change {
  std::array<std::uint16_t, facts::kMaxAddonSteps> addons{};
  std::uint8_t addonCount{};
  latin::Syncope syncope{};
  bool adverb{}, rawStem{};
  CRoute route{};
  std::uint8_t spellingRule{};
  latin::Age age = latin::Age::X;
  latin::Frequency frequency = latin::Frequency::A;
  std::uint8_t stemLength = 255, endingLength = 255;
  std::uint8_t stemTextLength{}, endingTextLength{};
  std::array<char, kMaxWordLength> stemText{}, endingText{};
  friend bool operator==(const Change&, const Change&) = default;
};
static_assert(std::is_trivially_copyable_v<Change>);

inline constexpr unsigned kMaximumChangeBytes =
    2 + 1 + 2 * facts::kMaxAddonSteps + 1 + 1 + 2 + 2 + 1 + kMaxWordLength + 1 + kMaxWordLength + 1;
static_assert(kMaximumChangeBytes <= sizeof(Change));
static_assert((kMaximumChangeBytes + 2) * 255 <= 65535);
static_assert(kMaxWordLength <= 31 && facts::kStemColumnCount == 4);

struct Match : Reading {
#ifdef WHITAKER_TRACE
  trace::Tags tags{};
#endif
};
static_assert(std::is_trivially_copyable_v<Match>);
#ifndef WHITAKER_TRACE
static_assert(sizeof(Match) == 8);
#endif

struct Work {
  Work() = default;
  Work(const Work&) = delete;
  Work& operator=(const Work&) = delete;
  ~Work() { std::free(matches); }
  Match* matches = nullptr;
  int count = 0, capacity = 0;
  std::uint8_t trimmed = 0;
  bool outOfMemory = false;
  std::string_view word{};
  std::array<Change, kMaximumChanges + 1> changes{};
  unsigned changeCount{};
};

[[nodiscard]] inline latin::Part part(const Reading& r) { return r.part; }
[[nodiscard]] inline latin::Analysis grammar(const Reading& r) { return latin::packed::grammarOf(r); }
[[nodiscard]] inline const Change& change(const Work& w, const Reading& r) {
  static constexpr Change empty{};
  return r.changes ? w.changes[unsigned(r.changes) - 1] : empty;
}
void setChange(Work& out, Reading& r, const Change& c);
[[nodiscard]] inline CRoute route(const Work& w, const Reading& r) { return change(w, r).route; }
[[nodiscard]] inline latin::Age age(const Work& w, const Reading& r) { return change(w, r).age; }
[[nodiscard]] inline latin::Frequency frequency(const Work& w, const Reading& r) { return change(w, r).frequency; }
[[nodiscard]] std::uint8_t stemLength(const Reading& r, const Change& c);
[[nodiscard]] std::uint8_t endingLength(const Work& w, const Reading& r);
[[nodiscard]] Match* newMatch(Work& out);
[[nodiscard]] inline bool foundOrTrimmed(const Work& out) { return out.count != 0 || out.trimmed; }
void fillFromImage(Work& out, Match& match, std::string_view spelling, const rel::Result& reading);
void addDictionaryReadings(Work& out, std::string_view spelling, bool (*accept)(const Reading&) = nullptr);
void prependAddon(Work& out, int first, const rel::Addon& addon);
void addAddonReading(Work& out, std::string_view word, const Reading& r, std::uint8_t orthLength,
                     std::uint16_t inflection, std::uint16_t addon, std::uint16_t secondAddon);

// INFO: Decode only when requested by the caller or the compatibility projection.
void decode(std::string_view word, const Reading& r, const Change& c, ReadingDetail& out);
// INFO: Variable-length change entries; the first two bytes are flags, followed by present fields.
[[nodiscard]] unsigned encodeChange(const Change& c, unsigned char* out);
[[nodiscard]] Change decodeChange(const unsigned char* in);

template <typename Keep> void keepOnly(Work& out, const int from, Keep&& keep) {
  int write = from;
  for (int i = from; i < out.count; ++i)
    if (keep(i))
      out.matches[write++] = out.matches[i];
  out.count = write;
}
} // namespace whitaker::parse
