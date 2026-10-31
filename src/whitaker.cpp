#include <whitaker.h>

#include "parse/adverb.hpp"
#include "parse/arena.hpp"
#include "parse/find_readings.hpp"
#include "parse/respellings.hpp"
#include "parse/result.hpp"

#include <cstdlib>
#include <cstring>
#include <new>
#include <string_view>

struct WhitakerStorage {
  whitaker::parse::Arena text;
  WhitakerWord* words = nullptr;
  std::size_t wordCapacity = 0;
  WhitakerRecord* records = nullptr;
  std::size_t recordCapacity = 0;
  WhitakerForm* forms = nullptr;
  std::size_t formCapacity = 0;

  ~WhitakerStorage() {
    std::free(words);
    std::free(records);
    std::free(forms);
  }
};

namespace {

using namespace whitaker::parse;

[[nodiscard]] bool looksLikeAName(std::string_view word) noexcept {
  return word.size() > 1 && word[0] >= 'A' && word[0] <= 'Z' && word[1] >= 'a' && word[1] <= 'z';
}

thread_local Work sWork;

// NOTE: Reading storage is reused; generated strings live in the supplied arena.
[[nodiscard]] bool readWord(std::string_view word, Arena& strings) noexcept {
  sWork.count = 0;
  sWork.trimmed = 0;
  sWork.outOfMemory = false;
  if (word.empty())
    return true;
  Text text{strings};
  findReadings(sWork, text, word);
  if (!foundOrTrimmed(sWork) && !looksLikeAName(word) && !addRespelledReadings(sWork, text, word))
    addRespelledEncliticReadings(sWork, text, word);
  addAdverbsOfAdjectives(sWork);
  return !sWork.outOfMemory && !text.outOfMemory();
}

template <typename T> [[nodiscard]] bool reserve(T*& items, std::size_t& capacity, std::size_t needed) noexcept {
  if (needed <= capacity)
    return true;
  std::size_t grown = capacity == 0 ? 16 : capacity;
  while (grown < needed)
    grown *= 2;
  auto* const moved = static_cast<T*>(std::realloc(items, grown * sizeof(T)));
  if (moved == nullptr)
    return false;
  items = moved;
  capacity = grown;
  return true;
}

[[nodiscard]] const char* keepText(Arena& arena, const char* text) noexcept {
  const std::size_t bytes = std::strlen(text) + 1;
  auto* const kept = static_cast<char*>(arena.allocate(bytes));
  if (kept != nullptr)
    std::memcpy(kept, text, bytes);
  return kept;
}

// NOTE: Null for none and when out of memory.
[[nodiscard]] const WhitakerAddonStep* keepAddons(Arena& arena, const WhitakerAddonStep* steps,
                                                  std::uint8_t count) noexcept {
  if (count == 0)
    return nullptr;
  auto* const kept =
      static_cast<WhitakerAddonStep*>(arena.allocate(count * sizeof(WhitakerAddonStep), alignof(WhitakerAddonStep)));
  if (kept != nullptr)
    std::memcpy(kept, steps, count * sizeof(WhitakerAddonStep));
  return kept;
}

// NOTE: WORDS starts a new block when the record, part, route, spelling rule or add-ons change.
[[nodiscard]] bool sameRecord(const Match& a, const Match& b) noexcept {
  if (a.entry.id != b.entry.id || a.grammar.part != b.grammar.part || a.route != b.route ||
      a.spellingRule != b.spellingRule || a.addonCount != b.addonCount)
    return false;
  for (std::uint8_t i = 0; i < a.addonCount; ++i)
    if (a.addons[i].id != b.addons[i].id || a.addons[i].kind != b.addons[i].kind)
      return false;
  return true;
}

[[nodiscard]] bool reserveFor(WhitakerResult& result, std::size_t words, std::size_t records,
                              std::size_t forms) noexcept {
  WhitakerStorage& storage = *result.storage;
  const bool reserved = reserve(storage.words, storage.wordCapacity, words) &&
                        reserve(storage.records, storage.recordCapacity, records) &&
                        reserve(storage.forms, storage.formCapacity, forms);
  result.words = storage.words;
  result.records = storage.records;
  result.forms = storage.forms;
  return reserved;
}

[[nodiscard]] bool appendWord(WhitakerResult& result) noexcept {
  WhitakerStorage& storage = *result.storage;
  const std::size_t count = static_cast<std::size_t>(sWork.count);
  if (!reserveFor(result, result.word_count + 1, result.record_count + count, result.form_count + count))
    return false;
  WhitakerWord& word = storage.words[result.word_count];
  word = {result.record_count, 0, sWork.trimmed};
  for (std::size_t i = 0; i < count; ++i) {
    const Match& reading = sWork.matches[i];
    if (i == 0 || !sameRecord(sWork.matches[i - 1], reading)) {
      WhitakerRecord& record = storage.records[word.first_record + word.record_count++];
      record = {.entry = reading.entry,
                .meaning = reading.meaning,
                .pos = reading.pos,
                .addons = keepAddons(storage.text, reading.addons, reading.addonCount),
                .addon_count = reading.addonCount,
                .route = reading.route,
                .spelling_rule = reading.spellingRule,
                .first_form = result.form_count + i,
                .form_count = 0};
      if (reading.addonCount != 0 && record.addons == nullptr)
        return false;
    }
    ++storage.records[word.first_record + word.record_count - 1].form_count;
    storage.forms[result.form_count + i] = {.orth = reading.orth,
                                            .ending = reading.ending,
                                            .grammar = reading.grammar,
                                            .inflection_age = reading.inflectionAge,
                                            .inflection_frequency = reading.inflectionFrequency};
  }
  ++result.word_count;
  result.record_count += word.record_count;
  result.form_count += count;
  return true;
}

[[nodiscard]] WhitakerStatus analyzeWord(const char* word, WhitakerResult* result, bool replace) {
  if (word == nullptr || result == nullptr || result->storage == nullptr)
    return WHITAKER_INVALID_ARGUMENT;
  const std::size_t size = std::strlen(word);
  if (size > WHITAKER_MAX_WORD_LENGTH)
    return WHITAKER_INPUT_TOO_LONG;
  // NOTE: Copied first: the word may live in the result, which this call may clear.
  char copy[WHITAKER_MAX_WORD_LENGTH];
  std::memcpy(copy, word, size);
  if (replace)
    whitaker_result_clear(result);
  Arena& strings = result->storage->text;
  const Arena::Mark mark = strings.mark();
  if (readWord({copy, size}, strings) && appendWord(*result))
    return WHITAKER_OK;
  strings.rewind(mark);
  return WHITAKER_OUT_OF_MEMORY;
}

} // namespace

extern "C" {

WhitakerResult* whitaker_result_new(void) {
  auto* const result = new (std::nothrow) WhitakerResult{};
  if (result == nullptr)
    return nullptr;
  result->storage = new (std::nothrow) WhitakerStorage{};
  if (result->storage == nullptr) {
    delete result;
    return nullptr;
  }
  return result;
}

void whitaker_result_free(WhitakerResult* result) {
  if (result == nullptr)
    return;
  delete result->storage;
  delete result;
}

void whitaker_result_clear(WhitakerResult* result) {
  if (result == nullptr || result->storage == nullptr)
    return;
  result->word_count = 0;
  result->record_count = 0;
  result->form_count = 0;
  result->storage->text.rewind({});
}

// NOTE: Roman numeral meanings belong to the result; dictionary meanings are shared.
WhitakerResult* whitaker_result_clone(const WhitakerResult* result) {
  if (result == nullptr || result->storage == nullptr)
    return nullptr;
  WhitakerResult* const copy = whitaker_result_new();
  if (copy == nullptr)
    return nullptr;
  Arena& strings = copy->storage->text;
  bool copied = reserveFor(*copy, result->word_count, result->record_count, result->form_count);
  for (std::size_t i = 0; copied && i < result->form_count; ++i) {
    WhitakerForm& form = copy->storage->forms[i];
    form = result->forms[i];
    form.orth = keepText(strings, form.orth);
    form.ending = keepText(strings, form.ending);
    copied = form.orth != nullptr && form.ending != nullptr;
  }
  for (std::size_t i = 0; copied && i < result->record_count; ++i) {
    WhitakerRecord& record = copy->storage->records[i];
    record = result->records[i];
    if (record.entry.id == WHITAKER_NO_RECORD)
      record.meaning = keepText(strings, record.meaning);
    record.addons = keepAddons(strings, record.addons, record.addon_count);
    copied = record.meaning != nullptr && (record.addon_count == 0 || record.addons != nullptr);
  }
  if (!copied) {
    whitaker_result_free(copy);
    return nullptr;
  }
  if (result->word_count != 0)
    std::memcpy(copy->storage->words, result->words, result->word_count * sizeof(WhitakerWord));
  copy->word_count = result->word_count;
  copy->record_count = result->record_count;
  copy->form_count = result->form_count;
  return copy;
}

WhitakerStatus whitaker_analyze(const char* word, WhitakerResult* result) { return analyzeWord(word, result, true); }

WhitakerStatus whitaker_analyze_append(const char* word, WhitakerResult* result) {
  return analyzeWord(word, result, false);
}

const char* whitaker_addon_spelling(uint16_t id) { return rel::addon(id).fix.data(); }

const char* whitaker_addon_meaning(uint16_t id) { return rel::addon(id).meaning; }

WhitakerSpellingRule whitaker_spelling_rule(uint8_t rule) { return whitaker::parse::spellingRule(rule); }

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
