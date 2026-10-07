#include <whitaker.h>

#include "facts.hpp"
#include "roman.hpp"
#include "search/addon_fallback.hpp"
#include "search/relationship_image.hpp"
#include "search/syncope_lookup.hpp"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace {

namespace rel = whitaker::relationship;

static_assert(WHITAKER_MAX_WORD_LENGTH == facts::kMaxWordCharacters, "the public word length must be the corpus fact");
static_assert(WHITAKER_MAX_MATCHES > rel::kMaximumResultsPerSpelling,
              "the result must hold every image match and a Roman numeral");
// Every reading owns its printed stem; a Roman numeral owns its spelling and one meaning line.
static_assert(WHITAKER_MAX_MATCHES * (WHITAKER_MAX_WORD_LENGTH + 1) + 64 <= WHITAKER_TEXT_BYTES,
              "the result's text must hold every string it can own");

[[nodiscard]] WhitakerGrammar grammarOf(const latin::Analysis& a) noexcept {
  return {.part = std::to_underlying(a.part),
          .which = a.which,
          .variant = a.variant.value,
          .case_of = std::to_underlying(a.caseOf),
          .number = std::to_underlying(a.number),
          .gender = std::to_underlying(a.gender),
          .comparison = std::to_underlying(a.comparison),
          .numeral_sort = std::to_underlying(a.numeralSort),
          .tense = std::to_underlying(a.tense),
          .voice = std::to_underlying(a.voice),
          .mood = std::to_underlying(a.mood),
          .person = a.person.value};
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

[[nodiscard]] WhitakerEntry entryOf(std::uint16_t index) noexcept {
  const rel::Entry entry = rel::entry(index);
  const rel::DictionaryRecord labels = rel::dictionary(index);
  return {.part = std::to_underlying(entry.part),
          .which = entry.which,
          .variant = entry.variant,
          .gender = std::to_underlying(entry.gender),
          .kind = entry.kind,
          .comparison = std::to_underlying(entry.comparison),
          .numeral_sort = std::to_underlying(entry.numeralSort),
          .area = std::to_underlying(entry.area),
          .geography = std::to_underlying(entry.geography),
          .source = std::to_underlying(entry.source),
          .numeral_value = entry.numeralValue.value,
          .age = std::to_underlying(labels.age),
          .frequency = std::to_underlying(labels.frequency)};
}

// The strings a result owns, appended to its `text`.
class Text {
public:
  explicit Text(WhitakerResult& out) noexcept : m_text{out.text} {}

  [[nodiscard]] std::size_t checkpoint() const noexcept { return m_used; }

  void rewind(std::size_t checkpoint) noexcept {
    assert(checkpoint <= m_used);
    m_used = checkpoint;
  }

  [[nodiscard]] const char* keep(std::string_view value) noexcept {
    assert(m_used + value.size() < WHITAKER_TEXT_BYTES);
    char* const result = m_text + m_used;
    // Filtering can move a retained stem earlier within this buffer.
    std::memmove(result, value.data(), value.size());
    result[value.size()] = '\0';
    m_used += value.size() + 1;
    return result;
  }

private:
  char* m_text;
  std::size_t m_used{};
};

void fillResult(WhitakerResult& out, Text& text, std::string_view spelling,
                std::optional<latin::Part> only = std::nullopt) noexcept {
  const rel::Program program = rel::lookup(spelling);
  std::uint32_t cursor = program.begin;
  std::uint16_t lexeme = std::numeric_limits<std::uint16_t>::max();
  for (unsigned i = 0; i < program.count && out.count < WHITAKER_MAX_MATCHES; ++i) {
    const rel::Result reading = rel::next(cursor, lexeme);
    if (only && reading.grammar.part != *only)
      continue;
    WhitakerMatch& match = out.matches[out.count++];
    match = {};
    char stem[facts::kMaxWordCharacters];
    match.orth = text.keep({stem, rel::printStem(spelling, reading.stemLength, reading.stemPattern, stem)});
    match.meaning = rel::dictionaryMeaning(reading.dictionary);
    match.pos = latin::name(reading.grammar.part);
    match.grammar = grammarOf(reading.grammar);
    match.entry = entryOf(reading.dictionary);
  }
}

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
  std::unreachable();
}

void prependAddon(WhitakerMatch& match, std::uint16_t id, latin::AddonKind kind) noexcept {
  if (match.addon_count >= WHITAKER_MAX_ADDON_STEPS)
    return;
  for (std::size_t i = match.addon_count; i > 0; --i)
    match.addons[i] = match.addons[i - 1];
  match.addons[0] = {id, addonKind(kind)};
  ++match.addon_count;
}

void prependAddon(WhitakerResult& out, int first, const rel::Addon& addon) noexcept {
  for (int i = first; i < out.count; ++i)
    prependAddon(out.matches[i], addon.id, addon.kind);
}

// INFO: A fix or packon reading prints a slice of the query, not a spelling the image holds, so the result owns that
//  string.
void add(WhitakerResult& out, Text& text, std::string_view word,
         std::span<const whitaker::FallbackMatch> found) noexcept {
  for (const whitaker::FallbackMatch& match : found) {
    if (out.count >= WHITAKER_MAX_MATCHES)
      break;
    WhitakerMatch& added = out.matches[out.count++];
    added = {};
    added.orth = text.keep(word.substr(match.orthOffset, match.orthLength));
    added.meaning = rel::dictionaryMeaning(match.dictionary);
    added.pos = latin::name(match.grammar.part);
    added.grammar = grammarOf(match.grammar);
    added.entry = entryOf(match.dictionary);
    for (std::uint8_t i = 0; i < match.addonCount; ++i)
      added.addons[i] = {match.addonId[i], addonKind(match.addonKind[i])};
    added.addon_count = match.addonCount;
  }
}

// INFO: A PACK entry is half a word the automaton holds no form of, so WORDS answers it inside Word, not as a fallback;
//  `quocumque` is reported both ways.
void addPackon(WhitakerResult& out, Text& text, std::string_view word) noexcept {
  add(out, text, word, whitaker::packonReadings(word));
}

[[nodiscard]] bool foldedEquals(std::string_view text, std::string_view fix) noexcept {
  return std::ranges::equal(text, fix, {}, facts::foldLetter);
}

// INFO: The Qu block of Word, word_package.adb:1755. Strip a tickon; what is left must be a qu/cu pronoun of three
//  letters or more, answered from PRON entries only. Ada leaves the loop on the first record that answers.
void addTickon(WhitakerResult& out, Text& text, std::string_view word) noexcept {
  for (const rel::Addon& tickon : rel::addonsOf(latin::AddonKind::Tickon)) {
    const std::string_view fix = tickon.fix;
    if (fix.empty() || word.size() <= fix.size() || !foldedEquals(word.substr(0, fix.size()), fix))
      continue;

    const std::string_view base = word.substr(fix.size());
    if (base.size() < 3 || !(foldedEquals(base.substr(0, 2), "qu") || foldedEquals(base.substr(0, 2), "cu")))
      continue;

    const int before = out.count;
    fillResult(out, text, base, latin::Part::PRON);

    if (out.count == before)
      addPackon(out, text, base);
    if (out.count != before) {
      prependAddon(out, before, tickon);
      return;
    }
  }
}

// INFO: Try_Tackons accepts every adjective; `cumque` is its only ADJ tackon and the source leaves its declension
//  unchecked.
[[nodiscard]] bool tackonAccepts(const WhitakerMatch& match, const rel::Addon& tackon) noexcept {
  using Part = latin::Part;
  switch (tackon.targetPart) {
  case Part::X:
    return true;
  case Part::N:
    return match.grammar.part == WHITAKER_PART_N && match.grammar.which <= tackon.target[0];
  case Part::PRON:
    return match.grammar.part == WHITAKER_PART_PRON && match.grammar.which <= tackon.target[0];
  case Part::ADJ:
    return match.grammar.part == WHITAKER_PART_ADJ;
  default:
    return false;
  }
}

void addRawWord(WhitakerResult& out, Text& text, std::string_view word) noexcept;

constexpr std::size_t kEncliticTackons = 4;

// INFO: Try_Tackons: the records after Enclitic's first four, first applicable record wins.
void addRegularTackon(WhitakerResult& out, Text& text, std::string_view word) noexcept {
  for (const rel::Addon& tackon : rel::addonsOf(latin::AddonKind::Tackon).subspan(kEncliticTackons)) {
    const std::string_view fix = tackon.fix;
    if (fix.empty() || word.size() <= fix.size() || !foldedEquals(word.substr(word.size() - fix.size()), fix))
      continue;

    const int before = out.count;
    const std::size_t checkpoint = text.checkpoint();
    addRawWord(out, text, word.substr(0, word.size() - fix.size()));
    // Reclaim rejected stems, and compact survivors in their original order. Their strings follow the checkpoint in the
    // same order as the matches.
    text.rewind(checkpoint);
    int kept = before;
    for (int i = before; i < out.count; ++i) {
      if (tackonAccepts(out.matches[i], tackon)) {
        const std::string_view stem{out.matches[i].orth};
        out.matches[kept] = out.matches[i];
        out.matches[kept++].orth = text.keep(stem);
      }
    }
    out.count = kept;
    if (kept != before) {
      prependAddon(out, before, tackon);
      return;
    }
  }
}

// INFO: Word: the Qu block, then the ordinary lookup, then the packon; a tickon reading does not stop the ordinary
//  lookup. Try_Tackons runs only when all of that answered nothing.
void addRawWord(WhitakerResult& out, Text& text, std::string_view word) noexcept {
  const int before = out.count;
  addTickon(out, text, word);
  fillResult(out, text, word);
  addPackon(out, text, word);
  if (out.count == before)
    addRegularTackon(out, text, word);
}

// INFO: Enclitic tries only -que when Word already answered, and all four special tackons for an unanswered word.
void addEnclitic(WhitakerResult& out, Text& text, std::string_view word, bool wordAnswered) noexcept {
  for (const rel::Addon& tackon : rel::addonsOf(latin::AddonKind::Tackon).first(wordAnswered ? 1 : kEncliticTackons)) {
    const std::string_view fix = tackon.fix;
    if (fix.empty() || word.size() <= fix.size() || !foldedEquals(word.substr(word.size() - fix.size()), fix))
      continue;
    const int before = out.count;
    addRawWord(out, text, word.substr(0, word.size() - fix.size()));
    if (out.count != before) {
      prependAddon(out, before, tackon);
      return;
    }
  }
}

// INFO: Pass skips syncope when any reading so far is a (5,1) verb, parse.adb:662-669. A V reading's Con is its
//  entry's, word_package.adb:1037-1050.
[[nodiscard]] bool hasConjugationFiveOne(const WhitakerResult& out) noexcept {
  for (int i = 0; i < out.count; ++i) {
    const WhitakerMatch& match = out.matches[i];
    if (match.grammar.part == WHITAKER_PART_V && match.entry.part == WHITAKER_PART_V && match.entry.which == 5 &&
        match.entry.variant == 1)
      return true;
  }
  return false;
}

// INFO: Word re-enters itself after removing a tackon but never re-enters Parse's Enclitic loop, so the two calls stay
//  separate.
void addTackonWord(WhitakerResult& out, Text& text, std::string_view word) noexcept {
  const int before = out.count;
  addRawWord(out, text, word);
  // INFO: Pass's "Pure SYNCOPE", parse.adb:671.
  // NOTE: Not ported: Enclitic's second Syncope of the whole word (578) and
  //  the fixes-only pass's (688), where a restored spelling may take a
  //  prefix or suffix. Syncope looks up the image only, not Word.
  if (!hasConjugationFiveOne(out))
    if (const std::string restored = whitaker::syncopeLookup(word); !restored.empty())
      fillResult(out, text, restored);
  addEnclitic(out, text, word, out.count != before);
}

// INFO: The ADDONS fallback only turns an unanswered word into answers.
void addFallback(WhitakerResult& out, Text& text, std::string_view word) noexcept {
  if (out.count != 0)
    return;
  add(out, text, word, whitaker::addonFallback(word));
}

void addRomanNumeral(WhitakerResult& out, Text& text, std::string_view word) noexcept {
  const unsigned value = whitaker::romanValue(word);
  if (value == 0)
    return;

  assert(out.count == 0);
  char meaning[64];
  std::snprintf(meaning, sizeof meaning, " %u  as a ROMAN NUMERAL;", value);
  WhitakerMatch& match = out.matches[0];
  match = {};
  match.orth = text.keep(word);
  match.meaning = text.keep(meaning);
  match.pos = latin::name(latin::Part::NUM);
  match.grammar = grammarOf({.part = latin::Part::NUM, .which = 2, .numeralSort = latin::NumeralSort::CARD});
  match.entry.part = WHITAKER_PART_NUM;
  match.entry.which = 2;
  match.entry.numeral_sort = WHITAKER_NUMERAL_SORT_CARD;
  // NOTE: romanValue stays below 5000, so the value fits; it is past Whitaker's 0..1000 NUMERAL_VALUE_TYPE for numerals
  //  over M.
  match.entry.numeral_value = static_cast<std::uint16_t>(value);
  out.count = 1;
}

} // namespace

extern "C" {

WhitakerStatus whitaker_init(void) { return WHITAKER_OK; }

WhitakerStatus whitaker_analyze(const char* word, WhitakerResult* out) {
  if (word == nullptr || out == nullptr)
    return WHITAKER_INVALID_ARGUMENT;
  const std::size_t size = std::strlen(word);
  if (size > WHITAKER_MAX_WORD_LENGTH)
    return WHITAKER_INPUT_TOO_LONG;
  // NOTE: Copied first: the word may live in out->text, which the readings
  //  overwrite.
  char copy[WHITAKER_MAX_WORD_LENGTH];
  std::memcpy(copy, word, size);
  const std::string_view query{copy, size};

  out->count = 0;
  if (query.empty())
    return WHITAKER_OK;
  Text text{*out};
  addRomanNumeral(*out, text, query);
  addTackonWord(*out, text, query);
  addFallback(*out, text, query);
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

const char* whitaker_addon_spelling(uint16_t id) { return rel::addon(id).fix.data(); }

const char* whitaker_addon_meaning(uint16_t id) { return rel::addon(id).meaning; }

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

size_t whitaker_describe(const WhitakerGrammar* grammar, char* out, size_t size) {
  if (grammar == nullptr || (out == nullptr && size != 0))
    return 0;
  return latin::describe(analysisOf(*grammar), out, size);
}

} // extern "C"
