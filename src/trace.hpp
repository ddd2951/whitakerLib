#pragma once

// INFO: What a lookup did, for the browser build: which passes ran, which pass found each reading and which of the
//  entry's stems its ending takes. Only with WHITAKER_TRACE.

// NOTE: A macro, not empty functions: even an empty inline call changes GCC's inlining of readWord, and without
//  WHITAKER_TRACE the library must compile to the same code as without this file.
#ifdef WHITAKER_TRACE
#define WHITAKER_TRACE_HOOK(call) whitaker::trace::call
#else
#define WHITAKER_TRACE_HOOK(call) static_cast<void>(0)
#endif

#ifdef WHITAKER_TRACE

#include "search/relationship_image.hpp"

#include <cstdint>
#include <string_view>
#include <vector>
#include <type_traits>

namespace whitaker::trace {

// NOTE: In the order readWord and findReadings run them.
enum class Pass : std::uint8_t {
  dictionary,
  romanNumeral,
  prefixVariants,
  syncope,
  enclitic,
  dictionaryWithAffixes,
  encliticWithAffixes,
  respelling,
  respelledEnclitic,
  adverbs,
};

[[nodiscard]] constexpr const char* name(const Pass value) {
  switch (value) {
    case Pass::dictionary:
      return "Dictionary";
    case Pass::romanNumeral:
      return "RomanNumeral";
    case Pass::prefixVariants:
      return "PrefixVariants";
    case Pass::syncope:
      return "Syncope";
    case Pass::enclitic:
      return "Enclitic";
    case Pass::dictionaryWithAffixes:
      return "DictionaryWithAffixes";
    case Pass::encliticWithAffixes:
      return "EncliticWithAffixes";
    case Pass::respelling:
      return "Respelling";
    case Pass::respelledEnclitic:
      return "RespelledEnclitic";
    case Pass::adverbs:
      return "Adverbs";
  }
  return nullptr;
}

inline constexpr std::uint8_t kNone = 0xFF;
inline constexpr std::uint16_t kNoEntry = 0xFFFF;

// INFO: Carried by each reading, so sorting and compacting keep it with its reading. `dictionary` and `description`
//  say which image row and inflection a reading came from, for stemKey.
struct Tags {
  std::uint8_t foundBy = kNone;
  std::uint16_t dictionary = kNoEntry;
  std::uint16_t description = 0;
};
static_assert(std::is_trivially_copyable_v<Tags>);

struct Step {
  Pass pass{};
  int readings{};
};
static_assert(std::is_trivially_copyable_v<Step>);

struct Log {
  std::vector<Step> steps{};
  std::vector<Tags> readings{};
};

inline thread_local Log tLog{};

inline void begin() {
  tLog.steps.clear();
  tLog.readings.clear();
}

// NOTE: Only notes where an image reading came from. Working out its stem key here, for every candidate, cost more
//  than the rest of the trace (80 against 53 ms over 51,300 words natively), so stemKey does it for the readings
//  returned.
template <typename Match> void source(Match& match, const relationship::Result& reading) {
  match.tags.dictionary = reading.dictionary;
  match.tags.description = reading.description;
}

// INFO: The stem key, 1 to 4, of the entry's class row with the reading's grammar and ending, as INFLECTS.LAT gives
//  it; kNone for a reading not from the image. Not the image's own stem number: that is the first of the entry's
//  stems with the same text, so apal apal gives 1 for NOM S N, which takes 2.
[[nodiscard]] inline std::uint8_t stemKey(const Tags& tags, const std::string_view ending) {
  namespace rel = relationship;
  if (tags.dictionary == kNoEntry)
    return kNone;
  const rel::Run spelled = rel::endingRun(ending);
  const rel::Run rows = rel::rows(rel::dictionaryClass(rel::dictionary(tags.dictionary).classId & rel::kClassIdMask));
  for (std::uint32_t r = rows.first; r < rel::end(rows); ++r) {
    const rel::ClassRowRecord row = rel::classRow(r);
    if (row.description == tags.description && row.inflect >= spelled.first && row.inflect < rel::end(spelled))
      return rel::inflection(row.inflect).stemKey;
  }
  return kNone;
}

// INFO: Readings without a pass yet came from this one; copies keep the pass of the reading they copy.
template <typename Work> void ran(const Pass pass, Work& out) {
  for (int i = 0; i < out.count; ++i)
    if (out.matches[i].tags.foundBy == kNone)
      out.matches[i].tags.foundBy = static_cast<std::uint8_t>(pass);
  tLog.steps.push_back({pass, out.count});
}

// INFO: After readWord's respelling, whose condition is left as it was. The enclitic pass logs Respelling first
//  (respellings.cpp); otherwise readings without a pass came from respelling the whole word.
template <typename Work> void respelled(Work& out) {
  if (!tLog.steps.empty() && tLog.steps.back().pass == Pass::respelling) {
    ran(Pass::respelledEnclitic, out);
    return;
  }
  for (int i = 0; i < out.count; ++i)
    if (out.matches[i].tags.foundBy == kNone) {
      ran(Pass::respelling, out);
      return;
    }
}

// NOTE: In the order appendWord makes forms, one per reading.
template <typename Work> void end(const Work& out) {
  for (int i = 0; i < out.count; ++i)
    tLog.readings.push_back(out.matches[i].tags);
}

} // namespace whitaker::trace

#endif
