#include <whitaker.h>

#include "facts.hpp"
#include "roman.hpp"
#include "search/addon_fallback.hpp"
#include "search/relationship_image.hpp"

#include <atomic>
#include <cassert>
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <functional>
#include <cstdio>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <string_view>

// Defined by the generated layout_image.S, which embeds data/whitaker.dat.
extern "C" const unsigned char whitaker_layout_image_start[];
extern "C" const unsigned char whitaker_layout_image_end[];

namespace {

namespace rel = whitaker::relationship;

static_assert(WHITAKER_MAX_WORD_LENGTH == facts::kMaxWordCharacters,
              "the public word length must be the corpus fact");
static_assert(WHITAKER_MAX_MATCHES > rel::kMaximumResultsPerSpelling,
              "the result must hold every image match and a Roman numeral");
// Every fix reading owns a stem slice of the word; a Roman numeral owns its
// spelling and one meaning line.
static_assert(WHITAKER_MAX_MATCHES * (WHITAKER_MAX_WORD_LENGTH + 1) + 64 <=
                  WHITAKER_TEXT_BYTES,
              "the result's text must hold every string it can own");

constexpr const char* kNumeralPos = "NUM";
constexpr const char* kNumeralInflection = "NUM D2 V0 X X X CARD";

enum class ImageState : std::uint8_t {
  Uninitialized,
  Initializing,
  Ready,
  Malformed,
};

struct Runtime {
  rel::Image image;
  std::atomic<ImageState> state{ImageState::Uninitialized};
};

[[nodiscard]] Runtime& runtime() noexcept {
  static Runtime s_value;
  return s_value;
}

[[nodiscard]] std::span<const std::byte> imageBytes() noexcept {
  const auto* first =
      reinterpret_cast<const std::byte*>(whitaker_layout_image_start);
  const auto* last =
      reinterpret_cast<const std::byte*>(whitaker_layout_image_end);
  return {first, static_cast<std::size_t>(last - first)};
}

// The strings a result owns, appended to its `text`.
class Text {
public:
  explicit Text(WhitakerResult& out) noexcept : m_text{out.text} {}

  [[nodiscard]] const char* keep(std::string_view value) noexcept {
    assert(m_used + value.size() < WHITAKER_TEXT_BYTES);
    char* const result = m_text + m_used;
    std::memcpy(result, value.data(), value.size());
    result[value.size()] = '\0';
    m_used += value.size() + 1;
    return result;
  }

private:
  char* m_text;
  std::size_t m_used{};
};

void fillResult(WhitakerResult& out, const rel::Image& image,
                rel::Image::Program program) noexcept {
  const int added = std::min(static_cast<int>(program.count),
                             WHITAKER_MAX_MATCHES - out.count);
  std::uint32_t cursor = program.begin;
  std::uint16_t dense = std::numeric_limits<std::uint16_t>::max();
  for (int i = 0; i < added; ++i) {
    const rel::Image::Result s = image.next(cursor, dense);
    WhitakerMatch& m = out.matches[out.count + i];
    m = {};
    m.orth = s.orth;
    m.meaning = s.meaning;
    m.pos = s.pos;
    m.inflection = s.inflection;
  }
  out.count += added;
}

[[nodiscard]] WhitakerAddonKind addonKind(rel::AddonKind kind) noexcept {
  switch (kind) {
  case rel::AddonKind::Tickon:
    return WHITAKER_ADDON_TICKON;
  case rel::AddonKind::Prefix:
    return WHITAKER_ADDON_PREFIX;
  case rel::AddonKind::Suffix:
    return WHITAKER_ADDON_SUFFIX;
  case rel::AddonKind::Tackon:
    return WHITAKER_ADDON_TACKON;
  case rel::AddonKind::Packon:
    return WHITAKER_ADDON_PACKON;
  }
  assert(false && "unknown ADDONS kind");
  return WHITAKER_ADDON_TACKON;
}

void prependAddon(WhitakerMatch& match, const char* spelling,
                  const char* meaning, rel::AddonKind kind) noexcept {
  assert(match.addon_count < WHITAKER_MAX_ADDON_STEPS);
  for (std::size_t i = match.addon_count; i > 0; --i)
    match.addons[i] = match.addons[i - 1];
  match.addons[0] = {spelling, meaning, addonKind(kind)};
  ++match.addon_count;
}

void prependAddon(WhitakerResult& out, int first,
                  const rel::Image::Addon& addon) noexcept {
  for (int i = first; i < out.count; ++i)
    prependAddon(out.matches[i], addon.fix, addon.meaning, addon.kind);
}

// INFO: A fix or packon reading prints a slice of the query, not a spelling
//  the image holds, so the result owns that string.
void add(WhitakerResult& out, Text& text, std::string_view word,
         std::span<const whitaker::FallbackMatch> found) noexcept {
  for (const whitaker::FallbackMatch& match : found) {
    if (out.count >= WHITAKER_MAX_MATCHES)
      break;
    WhitakerMatch& added = out.matches[out.count++];
    added = {};
    added.orth = text.keep(word.substr(match.orthOffset, match.orthLength));
    added.meaning = match.meaning;
    added.pos = match.pos;
    added.inflection = match.inflection;
    for (std::uint8_t step = match.addonCount; step > 0; --step) {
      const std::uint8_t index = static_cast<std::uint8_t>(step - 1);
      prependAddon(added, match.addonSpelling[index], match.addonMeaning[index],
                   match.addonKind[index]);
    }
  }
}

// INFO: A PACK entry is half a word the automaton holds no form of, so WORDS
//  answers it inside Word, not as a fallback; `quocumque` is reported both
//  ways.
void addPackon(WhitakerResult& out, Text& text, const rel::Image& image,
               std::string_view word) noexcept {
  add(out, text, word, whitaker::packonReadings(image, word));
}

[[nodiscard]] bool endsWithFolded(std::string_view word,
                                  std::string_view fix) noexcept {
  if (word.size() < fix.size())
    return false;
  const std::size_t first = word.size() - fix.size();
  for (std::size_t i = 0; i < fix.size(); ++i)
    if (facts::foldLetter(word[first + i]) != fix[i])
      return false;
  return true;
}

[[nodiscard]] bool startsWithFolded(std::string_view word,
                                    std::string_view fix) noexcept {
  if (word.size() < fix.size())
    return false;
  for (std::size_t i = 0; i < fix.size(); ++i)
    if (facts::foldLetter(word[i]) != fix[i])
      return false;
  return true;
}

// INFO: The Qu block of Word, word_package.adb:1755. Strip a tickon; what is
//  left must be a qu/cu pronoun of three letters or more, answered from PRON
//  entries only. Ada leaves the loop on the first record that answers.
void addTickon(WhitakerResult& out, Text& text, const rel::Image& image,
               std::string_view word) noexcept {
  for (std::uint32_t index = 0; index < image.addonCount(); ++index) {
    const auto tickon = image.addon(index);
    if (tickon.kind != rel::AddonKind::Tickon)
      continue;
    const std::string_view fix{tickon.fix};
    if (fix.empty() || word.size() <= fix.size() ||
        !startsWithFolded(word, fix))
      continue;

    const std::string_view base = word.substr(fix.size());
    if (base.size() < 3 ||
        !(startsWithFolded(base, "qu") || startsWithFolded(base, "cu")))
      continue;

    const int before = out.count;
    fillResult(out, image, image.lookup(base));
    int kept = before;
    for (int i = before; i < out.count; ++i)
      if (std::string_view{out.matches[i].pos} == "PRON")
        out.matches[kept++] = out.matches[i];
    out.count = kept;

    if (out.count == before)
      addPackon(out, text, image, base);
    if (out.count != before) {
      prependAddon(out, before, tickon);
      return;
    }
  }
}

[[nodiscard]] bool declAtMost(const char* inflection,
                              std::uint16_t limit) noexcept {
  const std::string_view text{inflection};
  const std::size_t marker = text.find(" D");
  if (marker == std::string_view::npos)
    return false;
  const char* begin = text.data() + marker + 2;
  const char* end = text.data() + text.size();
  unsigned value = 0;
  const auto [where, error] = std::from_chars(begin, end, value);
  return error == std::errc{} && where != begin && value <= limit;
}

// INFO: Try_Tackons accepts every adjective; `cumque` is its only ADJ tackon
//  and the source leaves its declension unchecked.
[[nodiscard]] bool tackonAccepts(const WhitakerMatch& match,
                                 const rel::Image::Addon& tackon) noexcept {
  using Part = rel::Part;
  switch (tackon.targetPart) {
  case Part::Any:
    return true;
  case Part::N:
    return std::string_view{match.pos} == "N" &&
           declAtMost(match.inflection, tackon.target[0]);
  case Part::PRON:
    return std::string_view{match.pos} == "PRON" &&
           declAtMost(match.inflection, tackon.target[0]);
  case Part::ADJ:
    return std::string_view{match.pos} == "ADJ";
  default:
    return false;
  }
}

void addRawWord(WhitakerResult& out, Text& text, const rel::Image& image,
                std::string_view word) noexcept;

// INFO: Try_Tackons: the records after Enclitic's first four, first
//  applicable record wins.
void addRegularTackon(WhitakerResult& out, Text& text, const rel::Image& image,
                      std::string_view word) noexcept {
  std::uint32_t ordinal = 0;
  for (std::uint32_t index = 0; index < image.addonCount(); ++index) {
    const auto tackon = image.addon(index);
    if (tackon.kind != rel::AddonKind::Tackon)
      continue;
    ++ordinal;
    if (ordinal <= 4)
      continue;
    const std::string_view fix{tackon.fix};
    if (fix.empty() || word.size() <= fix.size() || !endsWithFolded(word, fix))
      continue;

    const int before = out.count;
    addRawWord(out, text, image, word.substr(0, word.size() - fix.size()));
    int kept = before;
    for (int i = before; i < out.count; ++i) {
      if (tackonAccepts(out.matches[i], tackon))
        out.matches[kept++] = out.matches[i];
    }
    out.count = kept;
    if (kept != before) {
      prependAddon(out, before, tackon);
      return;
    }
  }
}

// INFO: Word: the Qu block, then the ordinary lookup, then the packon; a
//  tickon reading does not stop the ordinary lookup. Try_Tackons runs only
//  when all of that answered nothing.
void addRawWord(WhitakerResult& out, Text& text, const rel::Image& image,
                std::string_view word) noexcept {
  const int before = out.count;
  addTickon(out, text, image, word);
  fillResult(out, image, image.lookup(word));
  addPackon(out, text, image, word);
  if (out.count == before)
    addRegularTackon(out, text, image, word);
}

// INFO: Enclitic tries only -que when Word already answered, and all four
//  special tackons for an unanswered word.
void addEnclitic(WhitakerResult& out, Text& text, const rel::Image& image,
                 std::string_view word, bool wordAnswered) noexcept {
  std::uint32_t ordinal = 0;
  for (std::uint32_t index = 0; index < image.addonCount(); ++index) {
    const auto tackon = image.addon(index);
    if (tackon.kind != rel::AddonKind::Tackon)
      continue;
    ++ordinal;
    if (ordinal > 4)
      return;
    if (wordAnswered && ordinal > 1)
      return;
    const std::string_view fix{tackon.fix};
    if (fix.empty() || word.size() <= fix.size() || !endsWithFolded(word, fix))
      continue;
    const int before = out.count;
    addRawWord(out, text, image, word.substr(0, word.size() - fix.size()));
    if (out.count != before) {
      prependAddon(out, before, tackon);
      return;
    }
  }
}

// INFO: Word re-enters itself after removing a tackon but never re-enters
//  Parse's Enclitic loop, so the two calls stay separate.
void addTackonWord(WhitakerResult& out, Text& text, const rel::Image& image,
                   std::string_view word) noexcept {
  const int before = out.count;
  addRawWord(out, text, image, word);
  addEnclitic(out, text, image, word, out.count != before);
}

// INFO: The ADDONS fallback only turns an unanswered word into answers.
void addFallback(WhitakerResult& out, Text& text, const rel::Image& image,
                 std::string_view word) noexcept {
  if (out.count != 0)
    return;
  add(out, text, word, whitaker::addonFallback(image, word));
}

void addRomanNumeral(WhitakerResult& out, Text& text,
                     std::string_view word) noexcept {
  const unsigned value = whitaker::romanValue(word);
  if (value == 0)
    return;

  assert(out.count == 0);
  char meaning[64];
  std::snprintf(meaning, sizeof meaning, " %u  as a ROMAN NUMERAL;", value);
  out.matches[0] = {};
  out.matches[0].orth = text.keep(word);
  out.matches[0].meaning = text.keep(meaning);
  out.matches[0].pos = kNumeralPos;
  out.matches[0].inflection = kNumeralInflection;
  out.count = 1;
}

} // namespace

extern "C" {

WhitakerStatus whitaker_init(void) {
  Runtime& instance = runtime();
  for (;;) {
    ImageState state = instance.state.load(std::memory_order_acquire);
    if (state == ImageState::Ready)
      return WHITAKER_OK;
    if (state == ImageState::Malformed)
      return WHITAKER_MALFORMED_IMAGE;
    if (state == ImageState::Initializing) {
      instance.state.wait(state, std::memory_order_acquire);
      continue;
    }
    if (!instance.state.compare_exchange_weak(state, ImageState::Initializing,
                                              std::memory_order_acq_rel,
                                              std::memory_order_acquire))
      continue;

    std::string failure;
    if (!instance.image.loadSpan(imageBytes(), failure)) {
      instance.state.store(ImageState::Malformed, std::memory_order_release);
      instance.state.notify_all();
      return WHITAKER_MALFORMED_IMAGE;
    }
    instance.state.store(ImageState::Ready, std::memory_order_release);
    instance.state.notify_all();
    return WHITAKER_OK;
  }
}

WhitakerStatus whitaker_analyze(const char* word, WhitakerResult* out) {
  if (word == nullptr || out == nullptr)
    return WHITAKER_INVALID_ARGUMENT;
  const std::string_view query{word};
  if (query.size() > WHITAKER_MAX_WORD_LENGTH)
    return WHITAKER_INPUT_TOO_LONG;

  Runtime& instance = runtime();
  const ImageState state = instance.state.load(std::memory_order_acquire);
  if (state == ImageState::Malformed)
    return WHITAKER_MALFORMED_IMAGE;
  if (state != ImageState::Ready)
    return WHITAKER_NOT_INITIALIZED;

  out->count = 0;
  if (query.empty())
    return WHITAKER_OK;
  Text text{*out};
  addRomanNumeral(*out, text, query);
  addTackonWord(*out, text, instance.image, query);
  addFallback(*out, text, instance.image, query);
  return WHITAKER_OK;
}

void whitaker_result_copy(WhitakerResult* dst, const WhitakerResult* src) {
  if (dst == nullptr || src == nullptr || dst == src)
    return;
  std::memcpy(dst, src, sizeof *dst);
  const std::less<const char*> before;
  const auto rebase = [&](const char*& s) {
    if (!before(s, src->text) && before(s, src->text + WHITAKER_TEXT_BYTES))
      s = dst->text + (s - src->text);
  };
  for (int i = 0; i < dst->count; ++i) {
    WhitakerMatch& m = dst->matches[i];
    rebase(m.orth);
    rebase(m.meaning);
    rebase(m.pos);
    rebase(m.inflection);
    for (uint8_t a = 0; a < m.addon_count; ++a) {
      rebase(m.addons[a].spelling);
      rebase(m.addons[a].meaning);
    }
  }
}

} // extern "C"
