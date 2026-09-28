#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WHITAKER_VERSION_MAJOR 0
#define WHITAKER_VERSION_MINOR 2

#if defined(__GNUC__)
#define WHITAKER_API __attribute__((visibility("default")))
#else
#define WHITAKER_API
#endif

typedef enum {
  WHITAKER_OK = 0,
  WHITAKER_NOT_INITIALIZED,
  WHITAKER_INVALID_ARGUMENT,
  WHITAKER_INPUT_TOO_LONG,
  WHITAKER_MALFORMED_IMAGE,
} WhitakerStatus;

#define WHITAKER_MAX_WORD_LENGTH 24
#define WHITAKER_MAX_MATCHES 256
// NOTE: Tickon, tackon and packon steps each remove at least two letters of
//  a word of at most WHITAKER_MAX_WORD_LENGTH, and the image is refused
//  otherwise; prefix and suffix steps come alone, two at most.
#define WHITAKER_MAX_ADDON_STEPS (WHITAKER_MAX_WORD_LENGTH / 2)
#define WHITAKER_TEXT_BYTES 8192

// NOTE: These values match latin/latin.hpp; whitaker.cpp checks each one.
typedef enum {
  WHITAKER_PART_X = 0,
  WHITAKER_PART_N = 1,
  WHITAKER_PART_PRON = 2,
  WHITAKER_PART_V = 3,
  WHITAKER_PART_ADJ = 4,
  WHITAKER_PART_ADV = 5,
  WHITAKER_PART_PREP = 6,
  WHITAKER_PART_NUM = 7,
  WHITAKER_PART_CONJ = 8,
  WHITAKER_PART_INTERJ = 9,
  WHITAKER_PART_PACK = 10,
  WHITAKER_PART_SUPINE = 11,
  WHITAKER_PART_VPAR = 12,
  WHITAKER_PART_NONE = 13,
} WhitakerPart;

typedef enum {
  WHITAKER_CASE_X = 0,
  WHITAKER_CASE_NOM = 1,
  WHITAKER_CASE_VOC = 2,
  WHITAKER_CASE_GEN = 3,
  WHITAKER_CASE_LOC = 4,
  WHITAKER_CASE_DAT = 5,
  WHITAKER_CASE_ABL = 6,
  WHITAKER_CASE_ACC = 7,
} WhitakerCase;

typedef enum {
  WHITAKER_NUMBER_X = 0,
  WHITAKER_NUMBER_S = 1,
  WHITAKER_NUMBER_P = 2,
} WhitakerNumber;

typedef enum {
  WHITAKER_GENDER_X = 0,
  WHITAKER_GENDER_M = 1,
  WHITAKER_GENDER_F = 2,
  WHITAKER_GENDER_N = 3,
  WHITAKER_GENDER_C = 4,
} WhitakerGender;

typedef enum {
  WHITAKER_COMPARISON_X = 0,
  WHITAKER_COMPARISON_POS = 1,
  WHITAKER_COMPARISON_COMP = 2,
  WHITAKER_COMPARISON_SUPER = 3,
} WhitakerComparison;

typedef enum {
  WHITAKER_NUMERAL_SORT_X = 0,
  WHITAKER_NUMERAL_SORT_CARD = 1,
  WHITAKER_NUMERAL_SORT_ORD = 2,
  WHITAKER_NUMERAL_SORT_DIST = 3,
  WHITAKER_NUMERAL_SORT_ADVERB = 4,
} WhitakerNumeralSort;

typedef enum {
  WHITAKER_TENSE_X = 0,
  WHITAKER_TENSE_PRES = 1,
  WHITAKER_TENSE_IMPF = 2,
  WHITAKER_TENSE_FUT = 3,
  WHITAKER_TENSE_PERF = 4,
  WHITAKER_TENSE_PLUP = 5,
  WHITAKER_TENSE_FUTP = 6,
} WhitakerTense;

typedef enum {
  WHITAKER_VOICE_X = 0,
  WHITAKER_VOICE_ACTIVE = 1,
  WHITAKER_VOICE_PASSIVE = 2,
} WhitakerVoice;

typedef enum {
  WHITAKER_MOOD_X = 0,
  WHITAKER_MOOD_IND = 1,
  WHITAKER_MOOD_SUB = 2,
  WHITAKER_MOOD_IMP = 3,
  WHITAKER_MOOD_INF = 4,
  WHITAKER_MOOD_PPL = 5,
} WhitakerMood;

typedef enum {
  WHITAKER_NOUN_KIND_X = 0,
  WHITAKER_NOUN_KIND_S = 1,
  WHITAKER_NOUN_KIND_M = 2,
  WHITAKER_NOUN_KIND_A = 3,
  WHITAKER_NOUN_KIND_G = 4,
  WHITAKER_NOUN_KIND_N = 5,
  WHITAKER_NOUN_KIND_P = 6,
  WHITAKER_NOUN_KIND_T = 7,
  WHITAKER_NOUN_KIND_L = 8,
  WHITAKER_NOUN_KIND_W = 9,
  WHITAKER_NOUN_KIND_p = 10,
  WHITAKER_NOUN_KIND_t = 11,
  WHITAKER_NOUN_KIND_w = 12,
  WHITAKER_NOUN_KIND_x = 13,
} WhitakerNounKind;

typedef enum {
  WHITAKER_PRONOUN_KIND_X = 0,
  WHITAKER_PRONOUN_KIND_PERS = 1,
  WHITAKER_PRONOUN_KIND_REL = 2,
  WHITAKER_PRONOUN_KIND_REFLEX = 3,
  WHITAKER_PRONOUN_KIND_DEMONS = 4,
  WHITAKER_PRONOUN_KIND_INTERR = 5,
  WHITAKER_PRONOUN_KIND_INDEF = 6,
  WHITAKER_PRONOUN_KIND_ADJECT = 7,
} WhitakerPronounKind;

typedef enum {
  WHITAKER_PACKON_KIND_X = 0,
  WHITAKER_PACKON_KIND_INTERR = 1,
  WHITAKER_PACKON_KIND_REL = 2,
  WHITAKER_PACKON_KIND_INDEF = 3,
  WHITAKER_PACKON_KIND_ADJECT = 4,
} WhitakerPackonKind;

typedef enum {
  WHITAKER_VERB_KIND_X = 0,
  WHITAKER_VERB_KIND_TO_BE = 1,
  WHITAKER_VERB_KIND_TO_BEING = 2,
  WHITAKER_VERB_KIND_GEN = 3,
  WHITAKER_VERB_KIND_DAT = 4,
  WHITAKER_VERB_KIND_ABL = 5,
  WHITAKER_VERB_KIND_TRANS = 6,
  WHITAKER_VERB_KIND_INTRANS = 7,
  WHITAKER_VERB_KIND_IMPERS = 8,
  WHITAKER_VERB_KIND_DEP = 9,
  WHITAKER_VERB_KIND_SEMIDEP = 10,
  WHITAKER_VERB_KIND_PERFDEF = 11,
} WhitakerVerbKind;

typedef enum {
  WHITAKER_AGE_X = 0,
  WHITAKER_AGE_A = 1,
  WHITAKER_AGE_B = 2,
  WHITAKER_AGE_C = 3,
  WHITAKER_AGE_D = 4,
  WHITAKER_AGE_E = 5,
  WHITAKER_AGE_F = 6,
  WHITAKER_AGE_G = 7,
  WHITAKER_AGE_H = 8,
} WhitakerAge;

typedef enum {
  WHITAKER_FREQUENCY_X = 0,
  WHITAKER_FREQUENCY_A = 1,
  WHITAKER_FREQUENCY_B = 2,
  WHITAKER_FREQUENCY_C = 3,
  WHITAKER_FREQUENCY_D = 4,
  WHITAKER_FREQUENCY_E = 5,
  WHITAKER_FREQUENCY_F = 6,
  WHITAKER_FREQUENCY_I = 7,
  WHITAKER_FREQUENCY_M = 8,
  WHITAKER_FREQUENCY_N = 9,
} WhitakerFrequency;

typedef enum {
  WHITAKER_AREA_X = 0,
  WHITAKER_AREA_A = 1,
  WHITAKER_AREA_B = 2,
  WHITAKER_AREA_D = 3,
  WHITAKER_AREA_E = 4,
  WHITAKER_AREA_G = 5,
  WHITAKER_AREA_L = 6,
  WHITAKER_AREA_P = 7,
  WHITAKER_AREA_S = 8,
  WHITAKER_AREA_T = 9,
  WHITAKER_AREA_W = 10,
  WHITAKER_AREA_Y = 11,
} WhitakerArea;

typedef enum {
  WHITAKER_GEOGRAPHY_X = 0,
  WHITAKER_GEOGRAPHY_A = 1,
  WHITAKER_GEOGRAPHY_B = 2,
  WHITAKER_GEOGRAPHY_C = 3,
  WHITAKER_GEOGRAPHY_D = 4,
  WHITAKER_GEOGRAPHY_E = 5,
  WHITAKER_GEOGRAPHY_F = 6,
  WHITAKER_GEOGRAPHY_G = 7,
  WHITAKER_GEOGRAPHY_H = 8,
  WHITAKER_GEOGRAPHY_I = 9,
  WHITAKER_GEOGRAPHY_J = 10,
  WHITAKER_GEOGRAPHY_K = 11,
  WHITAKER_GEOGRAPHY_N = 12,
  WHITAKER_GEOGRAPHY_P = 13,
  WHITAKER_GEOGRAPHY_Q = 14,
  WHITAKER_GEOGRAPHY_R = 15,
  WHITAKER_GEOGRAPHY_S = 16,
  WHITAKER_GEOGRAPHY_U = 17,
} WhitakerGeography;

typedef enum {
  WHITAKER_SOURCE_X = 0,
  WHITAKER_SOURCE_A = 1,
  WHITAKER_SOURCE_B = 2,
  WHITAKER_SOURCE_C = 3,
  WHITAKER_SOURCE_D = 4,
  WHITAKER_SOURCE_E = 5,
  WHITAKER_SOURCE_F = 6,
  WHITAKER_SOURCE_G = 7,
  WHITAKER_SOURCE_H = 8,
  WHITAKER_SOURCE_I = 9,
  WHITAKER_SOURCE_J = 10,
  WHITAKER_SOURCE_K = 11,
  WHITAKER_SOURCE_L = 12,
  WHITAKER_SOURCE_M = 13,
  WHITAKER_SOURCE_N = 14,
  WHITAKER_SOURCE_O = 15,
  WHITAKER_SOURCE_P = 16,
  WHITAKER_SOURCE_Q = 17,
  WHITAKER_SOURCE_R = 18,
  WHITAKER_SOURCE_S = 19,
  WHITAKER_SOURCE_T = 20,
  WHITAKER_SOURCE_U = 21,
  WHITAKER_SOURCE_V = 22,
  WHITAKER_SOURCE_W = 23,
  WHITAKER_SOURCE_Y = 24,
  WHITAKER_SOURCE_Z = 25,
} WhitakerSource;

// NOTE: Not a twin. The API numbers addon kinds from 1 in its own order and
//  leaves 0 for "no addon"; latin::AddonKind (Tickon = 0 ...) has no such
//  value. whitaker.cpp maps one to the other.
typedef enum {
  WHITAKER_ADDON_PREFIX = 1,
  WHITAKER_ADDON_SUFFIX,
  WHITAKER_ADDON_TACKON,
  WHITAKER_ADDON_PACKON,
  WHITAKER_ADDON_TICKON,
} WhitakerAddonKind;

// NOTE: `kind` holds a WhitakerAddonKind. `id` names one of the library's
//  addon records, for whitaker_addon_spelling and whitaker_addon_meaning.
//  An id is only valid for the library that returned it; don't store it.
typedef struct {
  uint16_t id;
  uint8_t kind;
} WhitakerAddonStep;

// NOTE: Each field holds the value of the enum it names (`part` a
//  WhitakerPart, `case_of` a WhitakerCase, ...), one byte each. `which` is
//  the declension or conjugation and `variant` its variant; a field the
//  reading has no use for is 0.
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

// NOTE: Each enum field holds its matching Whitaker* value. `which` is a
//  declension for N, PRON, PACK, ADJ and NUM, or a conjugation for V. `kind`
//  is NounKind for N, PronounKind for PRON, PackonKind for PACK, and VerbKind
//  for V. Other unused fields are 0. `numeral_value` is the NUM value,
//  0..1000 for a dictionary numeral; a roman numeral's goes up to 4999.
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
} WhitakerEntry;

typedef enum {
  WHITAKER_FIELD_PART,
  WHITAKER_FIELD_CASE,
  WHITAKER_FIELD_NUMBER,
  WHITAKER_FIELD_GENDER,
  WHITAKER_FIELD_COMPARISON,
  WHITAKER_FIELD_NUMERAL_SORT,
  WHITAKER_FIELD_TENSE,
  WHITAKER_FIELD_VOICE,
  WHITAKER_FIELD_MOOD,
  WHITAKER_FIELD_NOUN_KIND,
  WHITAKER_FIELD_PRONOUN_KIND,
  WHITAKER_FIELD_PACKON_KIND,
  WHITAKER_FIELD_VERB_KIND,
  WHITAKER_FIELD_AGE,
  WHITAKER_FIELD_FREQUENCY,
  WHITAKER_FIELD_AREA,
  WHITAKER_FIELD_GEOGRAPHY,
  WHITAKER_FIELD_SOURCE,
} WhitakerField;

typedef struct {
  const char* orth;
  const char* meaning;
  const char* pos;
  WhitakerGrammar grammar;
  WhitakerEntry entry;
  WhitakerAddonStep addons[WHITAKER_MAX_ADDON_STEPS];
  uint8_t addon_count;
} WhitakerMatch;

// NOTE: Strings point into the library or into `text`. Keep the result and
//  they stay valid. Copy with whitaker_result_copy, not with `=`.
// NOTE: A word with more than WHITAKER_MAX_MATCHES readings is cut at that
//  many; no corpus so far comes near it.
typedef struct {
  WhitakerMatch matches[WHITAKER_MAX_MATCHES];
  int count;
  char text[WHITAKER_TEXT_BYTES];
} WhitakerResult;

// NOTE: Call before the first analyze. Calling it again is harmless, from
//  any thread.
WHITAKER_API WhitakerStatus whitaker_init(void);

// NOTE: Readings are in the same order WORDS prints them. An unknown word
//  gives WHITAKER_OK and count 0. On an error `out` is not touched.
// NOTE: Safe to call from several threads at once, each with its own `out`.
WHITAKER_API WhitakerStatus whitaker_analyze(const char* word,
                                             WhitakerResult* out);

// NOTE: Copies `src` into `dst` and points the copied strings into `dst`.
//  A null argument or the same object twice does nothing.
WHITAKER_API void whitaker_result_copy(WhitakerResult* dst,
                                       const WhitakerResult* src);

// NOTE: The name WORDS prints for `value` of `field`, "NOM" for
//  WHITAKER_CASE_NOM. NULL for a value the field doesn't have. The string is
//  the library's; don't free it.
WHITAKER_API const char* whitaker_name(WhitakerField field, uint8_t value);

// NOTE: The spelling and meaning of addon step `id`. NULL for an id the
//  library doesn't have, or before whitaker_init. The strings are the
//  library's; don't free them.
WHITAKER_API const char* whitaker_addon_spelling(uint16_t id);
WHITAKER_API const char* whitaker_addon_meaning(uint16_t id);

// NOTE: Writes the line WORDS prints for `grammar`, like snprintf: at most
//  `size` bytes, the terminator included, and returns the length of the
//  whole line. `out` may be NULL when `size` is 0. A null `grammar`
//  returns 0.
WHITAKER_API size_t whitaker_describe(const WhitakerGrammar* grammar, char* out,
                                      size_t size);

#ifdef __cplusplus
}
#endif
