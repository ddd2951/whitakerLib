#pragma once

#include "whitaker_types.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WHITAKER_VERSION_MAJOR 0
#define WHITAKER_VERSION_MINOR 3
#define WHITAKER_VERSION_PATCH 0

#if defined(_WIN32) && defined(WHITAKER_BUILDING)
#define WHITAKER_API __declspec(dllexport)
#elif defined(_WIN32)
#define WHITAKER_API __declspec(dllimport)
#elif defined(__GNUC__)
#define WHITAKER_API __attribute__((visibility("default")))
#else
#define WHITAKER_API
#endif

#define WHITAKER_MAX_WORD_LENGTH 24

// NOTE: Add-on ids are valid only for the library that returned them.
typedef struct {
  uint16_t id;
  uint8_t kind;
} WhitakerAddonStep;

// INFO: `which` is the declension or conjugation. Unused fields are 0.
typedef struct {
  uint8_t part;
  uint8_t which;
  uint8_t variant;
  uint8_t case_of;
  uint8_t number;
  uint8_t gender;
  uint8_t comparison;
  uint8_t numeral_sort;
  uint8_t tense;
  uint8_t voice;
  uint8_t mood;
  uint8_t person;
} WhitakerGrammar;

// INFO: `kind` uses NounKind, PronounKind, PackonKind or VerbKind according to `part`.
//  Unused fields are 0; Roman numerals use WHITAKER_NO_RECORD.
#define WHITAKER_GENERAL_RECORDS 39336
#define WHITAKER_NO_RECORD 0xFFFF
typedef struct {
  uint8_t part;
  uint8_t which;
  uint8_t variant;
  uint8_t gender;
  uint8_t kind;
  uint8_t comparison;
  uint8_t numeral_sort;
  uint8_t area;
  uint8_t geography;
  uint8_t source;
  uint16_t numeral_value;
  uint8_t age;
  uint8_t frequency;
  uint16_t id;
} WhitakerEntry;

typedef struct {
  const char* first;
  const char* second;
  uint8_t reversed;
  uint8_t route;
  uint8_t kind;
} WhitakerSpellingRule;

// INFO: `orth` is the stem after spelling changes; an absent ending is "".
typedef struct {
  const char* orth;
  const char* ending;
  WhitakerGrammar grammar;
  uint8_t inflection_age;
  uint8_t inflection_frequency;
} WhitakerForm;

// INFO: Add-ons are outermost first. `first_form` indexes result->forms.
typedef struct {
  WhitakerEntry entry;
  const char* meaning;
  const char* pos;
  const WhitakerAddonStep* addons;
  uint8_t addon_count;
  uint8_t route;
  uint8_t spelling_rule;
  size_t first_form;
  size_t form_count;
} WhitakerRecord;

// INFO: `first_record` indexes result->records. `trimmed` means rarer readings were omitted.
typedef struct {
  size_t first_record;
  size_t record_count;
  uint8_t trimmed;
} WhitakerWord;

// NOTE: Read-only. Arrays may move on each call; indexes remain valid. Strings and
//  add-ons remain valid until clear/free. All memory is managed by the library.
typedef struct WhitakerResult {
  const WhitakerWord* words;
  size_t word_count;
  const WhitakerRecord* records;
  size_t record_count;
  const WhitakerForm* forms;
  size_t form_count;
  struct WhitakerStorage* storage;
} WhitakerResult;

// INFO: NULL on allocation failure.
WHITAKER_API WhitakerResult* whitaker_result_new(void);
WHITAKER_API void whitaker_result_free(WhitakerResult* result);

// INFO: Removes all words without freeing the allocated memory.
WHITAKER_API void whitaker_result_clear(WhitakerResult* result);

// INFO: Independent copy; NULL on allocation failure or NULL input.
WHITAKER_API WhitakerResult* whitaker_result_clone(const WhitakerResult* result);

// INFO: Replaces the result. Unknown words return OK with no records.
// NOTE: Each thread needs its own result. Allocation failure may leave it empty.
WHITAKER_API WhitakerStatus whitaker_analyze(const char* word, WhitakerResult* result);

WHITAKER_API WhitakerStatus whitaker_analyze_append(const char* word, WhitakerResult* result);

// INFO: NULL for invalid values. Returned strings belong to the library.
WHITAKER_API const char* whitaker_name(WhitakerField field, uint8_t value);

// INFO: NULL for invalid ids. Returned strings belong to the library.
WHITAKER_API const char* whitaker_addon_spelling(uint16_t id);
WHITAKER_API const char* whitaker_addon_meaning(uint16_t id);

// INFO: Zeroed for invalid ids. Returned strings belong to the library.
WHITAKER_API WhitakerSpellingRule whitaker_spelling_rule(uint8_t rule);

// INFO: Formats WORDS' grammar line with snprintf semantics. NULL `out` is allowed
//  when size is 0; NULL `grammar` returns 0.
WHITAKER_API size_t whitaker_describe(const WhitakerGrammar* grammar, char* out, size_t size);

#ifdef __cplusplus
}
#endif
