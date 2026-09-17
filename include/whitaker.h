#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WHITAKER_VERSION_MAJOR 0
#define WHITAKER_VERSION_MINOR 1

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
#define WHITAKER_MAX_ADDON_STEPS 3
#define WHITAKER_TEXT_BYTES 8192

typedef enum {
  WHITAKER_ADDON_PREFIX = 1,
  WHITAKER_ADDON_SUFFIX,
  WHITAKER_ADDON_TACKON,
  WHITAKER_ADDON_PACKON,
  WHITAKER_ADDON_TICKON,
} WhitakerAddonKind;

typedef struct {
  const char* spelling;
  const char* meaning;
  WhitakerAddonKind kind;
} WhitakerAddonStep;

typedef struct {
  const char* orth;
  const char* meaning;
  const char* pos;
  const char* inflection;
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

#ifdef __cplusplus
}
#endif
