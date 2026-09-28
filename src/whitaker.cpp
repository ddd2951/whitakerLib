#include <whitaker.h>

#include "facts.hpp"
#include "roman.hpp"
#include "search/addon_fallback.hpp"
#include "search/relationship_image.hpp"

#include <atomic>
#include <cassert>
#include <algorithm>
#include <cstdint>
#include <functional>
#include <cstdio>
#include <cstring>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>

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

[[nodiscard]] WhitakerGrammar grammarOf(const latin::Analysis& a) noexcept {
  return {std::to_underlying(a.part),
          a.which,
          a.variant.value,
          std::to_underlying(a.caseOf),
          std::to_underlying(a.number),
          std::to_underlying(a.gender),
          std::to_underlying(a.comparison),
          std::to_underlying(a.numeralSort),
          std::to_underlying(a.tense),
          std::to_underlying(a.voice),
          std::to_underlying(a.mood),
          a.person.value};
}

[[nodiscard]] latin::Analysis analysisOf(const WhitakerGrammar& g) noexcept {
  return {.part = latin::Part{g.part},
          .which = g.which,
          .variant = {g.variant},
          .caseOf = latin::Case{g.case_of},
          .number = latin::Number{g.number},
          .gender = latin::Gender{g.gender},
          .comparison = latin::Comparison{g.comparison},
          .numeralSort = latin::NumeralSort{g.numeral_sort},
          .tense = latin::Tense{g.tense},
          .voice = latin::Voice{g.voice},
          .mood = latin::Mood{g.mood},
          .person = {g.person}};
}

[[nodiscard]] WhitakerEntry entryOf(const rel::Image& image,
                                    std::uint16_t index) noexcept {
  const rel::Entry entry = image.entry(index);
  const rel::Image::DictionaryMetadata labels = image.dictionaryMetadata(index);
  return {std::to_underlying(entry.part),
          entry.which,
          entry.variant,
          std::to_underlying(entry.gender),
          entry.kind,
          std::to_underlying(entry.comparison),
          std::to_underlying(entry.numeralSort),
          std::to_underlying(entry.area),
          std::to_underlying(entry.geography),
          std::to_underlying(entry.source),
          entry.numeralValue.value,
          std::to_underlying(labels.age),
          std::to_underlying(labels.frequency)};
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
    m.pos = latin::name(s.grammar.part);
    m.grammar = grammarOf(s.grammar);
    m.entry = entryOf(image, s.dictionary);
  }
  out.count += added;
}

static_assert(WHITAKER_PART_X == std::to_underlying(latin::Part::X) &&
              WHITAKER_PART_N == std::to_underlying(latin::Part::N) &&
              WHITAKER_PART_PRON == std::to_underlying(latin::Part::PRON) &&
              WHITAKER_PART_V == std::to_underlying(latin::Part::V) &&
              WHITAKER_PART_ADJ == std::to_underlying(latin::Part::ADJ) &&
              WHITAKER_PART_ADV == std::to_underlying(latin::Part::ADV) &&
              WHITAKER_PART_PREP == std::to_underlying(latin::Part::PREP) &&
              WHITAKER_PART_NUM == std::to_underlying(latin::Part::NUM) &&
              WHITAKER_PART_CONJ == std::to_underlying(latin::Part::CONJ) &&
              WHITAKER_PART_INTERJ == std::to_underlying(latin::Part::INTERJ) &&
              WHITAKER_PART_PACK == std::to_underlying(latin::Part::PACK) &&
              WHITAKER_PART_SUPINE == std::to_underlying(latin::Part::SUPINE) &&
              WHITAKER_PART_VPAR == std::to_underlying(latin::Part::VPAR) &&
              WHITAKER_PART_NONE == std::to_underlying(latin::Part::NONE));
static_assert(WHITAKER_CASE_X == std::to_underlying(latin::Case::X) &&
              WHITAKER_CASE_NOM == std::to_underlying(latin::Case::NOM) &&
              WHITAKER_CASE_VOC == std::to_underlying(latin::Case::VOC) &&
              WHITAKER_CASE_GEN == std::to_underlying(latin::Case::GEN) &&
              WHITAKER_CASE_LOC == std::to_underlying(latin::Case::LOC) &&
              WHITAKER_CASE_DAT == std::to_underlying(latin::Case::DAT) &&
              WHITAKER_CASE_ABL == std::to_underlying(latin::Case::ABL) &&
              WHITAKER_CASE_ACC == std::to_underlying(latin::Case::ACC));
static_assert(WHITAKER_NUMBER_X == std::to_underlying(latin::Number::X) &&
              WHITAKER_NUMBER_S == std::to_underlying(latin::Number::S) &&
              WHITAKER_NUMBER_P == std::to_underlying(latin::Number::P));
static_assert(WHITAKER_GENDER_X == std::to_underlying(latin::Gender::X) &&
              WHITAKER_GENDER_M == std::to_underlying(latin::Gender::M) &&
              WHITAKER_GENDER_F == std::to_underlying(latin::Gender::F) &&
              WHITAKER_GENDER_N == std::to_underlying(latin::Gender::N) &&
              WHITAKER_GENDER_C == std::to_underlying(latin::Gender::C));
static_assert(
    WHITAKER_COMPARISON_X == std::to_underlying(latin::Comparison::X) &&
    WHITAKER_COMPARISON_POS == std::to_underlying(latin::Comparison::POS) &&
    WHITAKER_COMPARISON_COMP == std::to_underlying(latin::Comparison::COMP) &&
    WHITAKER_COMPARISON_SUPER == std::to_underlying(latin::Comparison::SUPER));
static_assert(WHITAKER_NUMERAL_SORT_X ==
                  std::to_underlying(latin::NumeralSort::X) &&
              WHITAKER_NUMERAL_SORT_CARD ==
                  std::to_underlying(latin::NumeralSort::CARD) &&
              WHITAKER_NUMERAL_SORT_ORD ==
                  std::to_underlying(latin::NumeralSort::ORD) &&
              WHITAKER_NUMERAL_SORT_DIST ==
                  std::to_underlying(latin::NumeralSort::DIST) &&
              WHITAKER_NUMERAL_SORT_ADVERB ==
                  std::to_underlying(latin::NumeralSort::ADVERB));
static_assert(WHITAKER_TENSE_X == std::to_underlying(latin::Tense::X) &&
              WHITAKER_TENSE_PRES == std::to_underlying(latin::Tense::PRES) &&
              WHITAKER_TENSE_IMPF == std::to_underlying(latin::Tense::IMPF) &&
              WHITAKER_TENSE_FUT == std::to_underlying(latin::Tense::FUT) &&
              WHITAKER_TENSE_PERF == std::to_underlying(latin::Tense::PERF) &&
              WHITAKER_TENSE_PLUP == std::to_underlying(latin::Tense::PLUP) &&
              WHITAKER_TENSE_FUTP == std::to_underlying(latin::Tense::FUTP));
static_assert(WHITAKER_VOICE_X == std::to_underlying(latin::Voice::X) &&
              WHITAKER_VOICE_ACTIVE ==
                  std::to_underlying(latin::Voice::ACTIVE) &&
              WHITAKER_VOICE_PASSIVE ==
                  std::to_underlying(latin::Voice::PASSIVE));
static_assert(WHITAKER_MOOD_X == std::to_underlying(latin::Mood::X) &&
              WHITAKER_MOOD_IND == std::to_underlying(latin::Mood::IND) &&
              WHITAKER_MOOD_SUB == std::to_underlying(latin::Mood::SUB) &&
              WHITAKER_MOOD_IMP == std::to_underlying(latin::Mood::IMP) &&
              WHITAKER_MOOD_INF == std::to_underlying(latin::Mood::INF) &&
              WHITAKER_MOOD_PPL == std::to_underlying(latin::Mood::PPL));

static_assert(WHITAKER_NOUN_KIND_X == std::to_underlying(latin::NounKind::X) &&
              WHITAKER_NOUN_KIND_S == std::to_underlying(latin::NounKind::S) &&
              WHITAKER_NOUN_KIND_M == std::to_underlying(latin::NounKind::M) &&
              WHITAKER_NOUN_KIND_A == std::to_underlying(latin::NounKind::A) &&
              WHITAKER_NOUN_KIND_G == std::to_underlying(latin::NounKind::G) &&
              WHITAKER_NOUN_KIND_N == std::to_underlying(latin::NounKind::N) &&
              WHITAKER_NOUN_KIND_P == std::to_underlying(latin::NounKind::P) &&
              WHITAKER_NOUN_KIND_T == std::to_underlying(latin::NounKind::T) &&
              WHITAKER_NOUN_KIND_L == std::to_underlying(latin::NounKind::L) &&
              WHITAKER_NOUN_KIND_W == std::to_underlying(latin::NounKind::W) &&
              WHITAKER_NOUN_KIND_p == std::to_underlying(latin::NounKind::p) &&
              WHITAKER_NOUN_KIND_t == std::to_underlying(latin::NounKind::t) &&
              WHITAKER_NOUN_KIND_w == std::to_underlying(latin::NounKind::w) &&
              WHITAKER_NOUN_KIND_x == std::to_underlying(latin::NounKind::x));
static_assert(WHITAKER_PRONOUN_KIND_X ==
                  std::to_underlying(latin::PronounKind::X) &&
              WHITAKER_PRONOUN_KIND_PERS ==
                  std::to_underlying(latin::PronounKind::PERS) &&
              WHITAKER_PRONOUN_KIND_REL ==
                  std::to_underlying(latin::PronounKind::REL) &&
              WHITAKER_PRONOUN_KIND_REFLEX ==
                  std::to_underlying(latin::PronounKind::REFLEX) &&
              WHITAKER_PRONOUN_KIND_DEMONS ==
                  std::to_underlying(latin::PronounKind::DEMONS) &&
              WHITAKER_PRONOUN_KIND_INTERR ==
                  std::to_underlying(latin::PronounKind::INTERR) &&
              WHITAKER_PRONOUN_KIND_INDEF ==
                  std::to_underlying(latin::PronounKind::INDEF) &&
              WHITAKER_PRONOUN_KIND_ADJECT ==
                  std::to_underlying(latin::PronounKind::ADJECT));
static_assert(WHITAKER_PACKON_KIND_X ==
                  std::to_underlying(latin::PackonKind::X) &&
              WHITAKER_PACKON_KIND_INTERR ==
                  std::to_underlying(latin::PackonKind::INTERR) &&
              WHITAKER_PACKON_KIND_REL ==
                  std::to_underlying(latin::PackonKind::REL) &&
              WHITAKER_PACKON_KIND_INDEF ==
                  std::to_underlying(latin::PackonKind::INDEF) &&
              WHITAKER_PACKON_KIND_ADJECT ==
                  std::to_underlying(latin::PackonKind::ADJECT));
static_assert(
    WHITAKER_VERB_KIND_X == std::to_underlying(latin::VerbKind::X) &&
    WHITAKER_VERB_KIND_TO_BE == std::to_underlying(latin::VerbKind::TO_BE) &&
    WHITAKER_VERB_KIND_TO_BEING ==
        std::to_underlying(latin::VerbKind::TO_BEING) &&
    WHITAKER_VERB_KIND_GEN == std::to_underlying(latin::VerbKind::GEN) &&
    WHITAKER_VERB_KIND_DAT == std::to_underlying(latin::VerbKind::DAT) &&
    WHITAKER_VERB_KIND_ABL == std::to_underlying(latin::VerbKind::ABL) &&
    WHITAKER_VERB_KIND_TRANS == std::to_underlying(latin::VerbKind::TRANS) &&
    WHITAKER_VERB_KIND_INTRANS ==
        std::to_underlying(latin::VerbKind::INTRANS) &&
    WHITAKER_VERB_KIND_IMPERS == std::to_underlying(latin::VerbKind::IMPERS) &&
    WHITAKER_VERB_KIND_DEP == std::to_underlying(latin::VerbKind::DEP) &&
    WHITAKER_VERB_KIND_SEMIDEP ==
        std::to_underlying(latin::VerbKind::SEMIDEP) &&
    WHITAKER_VERB_KIND_PERFDEF == std::to_underlying(latin::VerbKind::PERFDEF));
static_assert(WHITAKER_AGE_X == std::to_underlying(latin::Age::X) &&
              WHITAKER_AGE_A == std::to_underlying(latin::Age::A) &&
              WHITAKER_AGE_B == std::to_underlying(latin::Age::B) &&
              WHITAKER_AGE_C == std::to_underlying(latin::Age::C) &&
              WHITAKER_AGE_D == std::to_underlying(latin::Age::D) &&
              WHITAKER_AGE_E == std::to_underlying(latin::Age::E) &&
              WHITAKER_AGE_F == std::to_underlying(latin::Age::F) &&
              WHITAKER_AGE_G == std::to_underlying(latin::Age::G) &&
              WHITAKER_AGE_H == std::to_underlying(latin::Age::H));
static_assert(WHITAKER_FREQUENCY_X == std::to_underlying(latin::Frequency::X) &&
              WHITAKER_FREQUENCY_A == std::to_underlying(latin::Frequency::A) &&
              WHITAKER_FREQUENCY_B == std::to_underlying(latin::Frequency::B) &&
              WHITAKER_FREQUENCY_C == std::to_underlying(latin::Frequency::C) &&
              WHITAKER_FREQUENCY_D == std::to_underlying(latin::Frequency::D) &&
              WHITAKER_FREQUENCY_E == std::to_underlying(latin::Frequency::E) &&
              WHITAKER_FREQUENCY_F == std::to_underlying(latin::Frequency::F) &&
              WHITAKER_FREQUENCY_I == std::to_underlying(latin::Frequency::I) &&
              WHITAKER_FREQUENCY_M == std::to_underlying(latin::Frequency::M) &&
              WHITAKER_FREQUENCY_N == std::to_underlying(latin::Frequency::N));
static_assert(WHITAKER_AREA_X == std::to_underlying(latin::Area::X) &&
              WHITAKER_AREA_A == std::to_underlying(latin::Area::A) &&
              WHITAKER_AREA_B == std::to_underlying(latin::Area::B) &&
              WHITAKER_AREA_D == std::to_underlying(latin::Area::D) &&
              WHITAKER_AREA_E == std::to_underlying(latin::Area::E) &&
              WHITAKER_AREA_G == std::to_underlying(latin::Area::G) &&
              WHITAKER_AREA_L == std::to_underlying(latin::Area::L) &&
              WHITAKER_AREA_P == std::to_underlying(latin::Area::P) &&
              WHITAKER_AREA_S == std::to_underlying(latin::Area::S) &&
              WHITAKER_AREA_T == std::to_underlying(latin::Area::T) &&
              WHITAKER_AREA_W == std::to_underlying(latin::Area::W) &&
              WHITAKER_AREA_Y == std::to_underlying(latin::Area::Y));
static_assert(WHITAKER_GEOGRAPHY_X == std::to_underlying(latin::Geography::X) &&
              WHITAKER_GEOGRAPHY_A == std::to_underlying(latin::Geography::A) &&
              WHITAKER_GEOGRAPHY_B == std::to_underlying(latin::Geography::B) &&
              WHITAKER_GEOGRAPHY_C == std::to_underlying(latin::Geography::C) &&
              WHITAKER_GEOGRAPHY_D == std::to_underlying(latin::Geography::D) &&
              WHITAKER_GEOGRAPHY_E == std::to_underlying(latin::Geography::E) &&
              WHITAKER_GEOGRAPHY_F == std::to_underlying(latin::Geography::F) &&
              WHITAKER_GEOGRAPHY_G == std::to_underlying(latin::Geography::G) &&
              WHITAKER_GEOGRAPHY_H == std::to_underlying(latin::Geography::H) &&
              WHITAKER_GEOGRAPHY_I == std::to_underlying(latin::Geography::I) &&
              WHITAKER_GEOGRAPHY_J == std::to_underlying(latin::Geography::J) &&
              WHITAKER_GEOGRAPHY_K == std::to_underlying(latin::Geography::K) &&
              WHITAKER_GEOGRAPHY_N == std::to_underlying(latin::Geography::N) &&
              WHITAKER_GEOGRAPHY_P == std::to_underlying(latin::Geography::P) &&
              WHITAKER_GEOGRAPHY_Q == std::to_underlying(latin::Geography::Q) &&
              WHITAKER_GEOGRAPHY_R == std::to_underlying(latin::Geography::R) &&
              WHITAKER_GEOGRAPHY_S == std::to_underlying(latin::Geography::S) &&
              WHITAKER_GEOGRAPHY_U == std::to_underlying(latin::Geography::U));
static_assert(WHITAKER_SOURCE_X == std::to_underlying(latin::Source::X) &&
              WHITAKER_SOURCE_A == std::to_underlying(latin::Source::A) &&
              WHITAKER_SOURCE_B == std::to_underlying(latin::Source::B) &&
              WHITAKER_SOURCE_C == std::to_underlying(latin::Source::C) &&
              WHITAKER_SOURCE_D == std::to_underlying(latin::Source::D) &&
              WHITAKER_SOURCE_E == std::to_underlying(latin::Source::E) &&
              WHITAKER_SOURCE_F == std::to_underlying(latin::Source::F) &&
              WHITAKER_SOURCE_G == std::to_underlying(latin::Source::G) &&
              WHITAKER_SOURCE_H == std::to_underlying(latin::Source::H) &&
              WHITAKER_SOURCE_I == std::to_underlying(latin::Source::I) &&
              WHITAKER_SOURCE_J == std::to_underlying(latin::Source::J) &&
              WHITAKER_SOURCE_K == std::to_underlying(latin::Source::K) &&
              WHITAKER_SOURCE_L == std::to_underlying(latin::Source::L) &&
              WHITAKER_SOURCE_M == std::to_underlying(latin::Source::M) &&
              WHITAKER_SOURCE_N == std::to_underlying(latin::Source::N) &&
              WHITAKER_SOURCE_O == std::to_underlying(latin::Source::O) &&
              WHITAKER_SOURCE_P == std::to_underlying(latin::Source::P) &&
              WHITAKER_SOURCE_Q == std::to_underlying(latin::Source::Q) &&
              WHITAKER_SOURCE_R == std::to_underlying(latin::Source::R) &&
              WHITAKER_SOURCE_S == std::to_underlying(latin::Source::S) &&
              WHITAKER_SOURCE_T == std::to_underlying(latin::Source::T) &&
              WHITAKER_SOURCE_U == std::to_underlying(latin::Source::U) &&
              WHITAKER_SOURCE_V == std::to_underlying(latin::Source::V) &&
              WHITAKER_SOURCE_W == std::to_underlying(latin::Source::W) &&
              WHITAKER_SOURCE_Y == std::to_underlying(latin::Source::Y) &&
              WHITAKER_SOURCE_Z == std::to_underlying(latin::Source::Z));

[[nodiscard]] std::uint8_t addonKind(latin::AddonKind kind) noexcept {
  switch (kind) {
  case latin::AddonKind::Tickon:
    return WHITAKER_ADDON_TICKON;
  case latin::AddonKind::Prefix:
    return WHITAKER_ADDON_PREFIX;
  case latin::AddonKind::Suffix:
    return WHITAKER_ADDON_SUFFIX;
  case latin::AddonKind::Tackon:
    return WHITAKER_ADDON_TACKON;
  case latin::AddonKind::Packon:
    return WHITAKER_ADDON_PACKON;
  }
  assert(false && "unknown ADDONS kind");
  return WHITAKER_ADDON_TACKON;
}

void prependAddon(WhitakerMatch& match, std::uint16_t id,
                  latin::AddonKind kind) noexcept {
  if (match.addon_count >= WHITAKER_MAX_ADDON_STEPS)
    return;
  for (std::size_t i = match.addon_count; i > 0; --i)
    match.addons[i] = match.addons[i - 1];
  match.addons[0] = {id, addonKind(kind)};
  ++match.addon_count;
}

void prependAddon(WhitakerResult& out, int first,
                  const rel::Image::Addon& addon) noexcept {
  for (int i = first; i < out.count; ++i)
    prependAddon(out.matches[i], addon.id, addon.kind);
}

// INFO: A fix or packon reading prints a slice of the query, not a spelling
//  the image holds, so the result owns that string.
void add(WhitakerResult& out, Text& text, const rel::Image& image,
         std::string_view word,
         std::span<const whitaker::FallbackMatch> found) noexcept {
  for (const whitaker::FallbackMatch& match : found) {
    if (out.count >= WHITAKER_MAX_MATCHES)
      break;
    WhitakerMatch& added = out.matches[out.count++];
    added = {};
    added.orth = text.keep(word.substr(match.orthOffset, match.orthLength));
    added.meaning = match.meaning;
    added.pos = latin::name(match.grammar.part);
    added.grammar = grammarOf(match.grammar);
    added.entry = entryOf(image, match.dictionary);
    for (std::uint8_t step = match.addonCount; step > 0; --step) {
      const std::uint8_t index = static_cast<std::uint8_t>(step - 1);
      prependAddon(added, match.addonId[index], match.addonKind[index]);
    }
  }
}

// INFO: A PACK entry is half a word the automaton holds no form of, so WORDS
//  answers it inside Word, not as a fallback; `quocumque` is reported both
//  ways.
void addPackon(WhitakerResult& out, Text& text, const rel::Image& image,
               std::string_view word) noexcept {
  add(out, text, image, word, whitaker::packonReadings(image, word));
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
  for (std::uint16_t index = 0; index < image.addonCount(); ++index) {
    const auto tickon = image.addon(index);
    if (tickon.kind != latin::AddonKind::Tickon)
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
      if (out.matches[i].grammar.part == WHITAKER_PART_PRON)
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

// INFO: Try_Tackons accepts every adjective; `cumque` is its only ADJ tackon
//  and the source leaves its declension unchecked.
[[nodiscard]] bool tackonAccepts(const WhitakerMatch& match,
                                 const rel::Image::Addon& tackon) noexcept {
  using Part = latin::Part;
  switch (tackon.targetPart) {
  case Part::X:
    return true;
  case Part::N:
    return match.grammar.part == WHITAKER_PART_N &&
           match.grammar.which <= tackon.target[0];
  case Part::PRON:
    return match.grammar.part == WHITAKER_PART_PRON &&
           match.grammar.which <= tackon.target[0];
  case Part::ADJ:
    return match.grammar.part == WHITAKER_PART_ADJ;
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
  for (std::uint16_t index = 0; index < image.addonCount(); ++index) {
    const auto tackon = image.addon(index);
    if (tackon.kind != latin::AddonKind::Tackon)
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
  for (std::uint16_t index = 0; index < image.addonCount(); ++index) {
    const auto tackon = image.addon(index);
    if (tackon.kind != latin::AddonKind::Tackon)
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
  add(out, text, image, word, whitaker::addonFallback(image, word));
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
  out.matches[0].pos = latin::name(latin::Part::NUM);
  out.matches[0].grammar = grammarOf({.part = latin::Part::NUM,
                                      .which = 2,
                                      .numeralSort = latin::NumeralSort::CARD});
  out.matches[0].entry.part = WHITAKER_PART_NUM;
  out.matches[0].entry.which = 2;
  out.matches[0].entry.numeral_sort = WHITAKER_NUMERAL_SORT_CARD;
  // NOTE: romanValue stays below 5000, so the value fits; it is past
  //  Whitaker's 0..1000 NUMERAL_VALUE_TYPE for numerals over M.
  out.matches[0].entry.numeral_value = static_cast<std::uint16_t>(value);
  out.count = 1;
}

[[nodiscard]] rel::Image::Addon addonOf(std::uint16_t id) noexcept {
  const Runtime& instance = runtime();
  if (instance.state.load(std::memory_order_acquire) != ImageState::Ready)
    return {};
  return instance.image.addon(id);
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
  }
}

const char* whitaker_addon_spelling(uint16_t id) { return addonOf(id).fix; }

const char* whitaker_addon_meaning(uint16_t id) { return addonOf(id).meaning; }

const char* whitaker_name(WhitakerField field, uint8_t value) {
  switch (field) {
  case WHITAKER_FIELD_PART:
    return latin::name(latin::Part{value});
  case WHITAKER_FIELD_CASE:
    return latin::name(latin::Case{value});
  case WHITAKER_FIELD_NUMBER:
    return latin::name(latin::Number{value});
  case WHITAKER_FIELD_GENDER:
    return latin::name(latin::Gender{value});
  case WHITAKER_FIELD_COMPARISON:
    return latin::name(latin::Comparison{value});
  case WHITAKER_FIELD_NUMERAL_SORT:
    return latin::name(latin::NumeralSort{value});
  case WHITAKER_FIELD_TENSE:
    return latin::name(latin::Tense{value});
  case WHITAKER_FIELD_VOICE:
    return latin::name(latin::Voice{value});
  case WHITAKER_FIELD_MOOD:
    return latin::name(latin::Mood{value});
  case WHITAKER_FIELD_NOUN_KIND:
    return latin::name(latin::NounKind{value});
  case WHITAKER_FIELD_PRONOUN_KIND:
    return latin::name(latin::PronounKind{value});
  case WHITAKER_FIELD_PACKON_KIND:
    return latin::name(latin::PackonKind{value});
  case WHITAKER_FIELD_VERB_KIND:
    return latin::name(latin::VerbKind{value});
  case WHITAKER_FIELD_AGE:
    return latin::name(latin::Age{value});
  case WHITAKER_FIELD_FREQUENCY:
    return latin::name(latin::Frequency{value});
  case WHITAKER_FIELD_AREA:
    return latin::name(latin::Area{value});
  case WHITAKER_FIELD_GEOGRAPHY:
    return latin::name(latin::Geography{value});
  case WHITAKER_FIELD_SOURCE:
    return latin::name(latin::Source{value});
  }
  return nullptr;
}

size_t whitaker_describe(const WhitakerGrammar* grammar, char* out,
                         size_t size) {
  if (grammar == nullptr || (out == nullptr && size != 0))
    return 0;
  return latin::describe(analysisOf(*grammar), out, size);
}

} // extern "C"
