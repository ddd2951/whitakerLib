/* Linked against test/malformed_image.S instead of data/whitaker.dat, so the
   library's own reader rejects its payload. */

#include <assert.h>
#include <stddef.h>

#include "whitaker.h"

int main(void) {
  WhitakerResult out;
  out.count = -1;

  assert(whitaker_init() == WHITAKER_MALFORMED_IMAGE);
  assert(whitaker_init() == WHITAKER_MALFORMED_IMAGE);

  /* Caller mistakes keep their own errors. */
  assert(whitaker_analyze(NULL, &out) == WHITAKER_INVALID_ARGUMENT);
  assert(whitaker_analyze("amo", NULL) == WHITAKER_INVALID_ARGUMENT);
  assert(whitaker_analyze("abcdefghijklmnopqrstuvwxy", &out) ==
         WHITAKER_INPUT_TOO_LONG);

  /* Everything else reports the image: a known word, an unknown word, and a
     Roman numeral that needs no image. The result is not touched. */
  assert(whitaker_analyze("amo", &out) == WHITAKER_MALFORMED_IMAGE);
  assert(whitaker_analyze("aqqqq", &out) == WHITAKER_MALFORMED_IMAGE);
  assert(whitaker_analyze("mcmlxxxiv", &out) == WHITAKER_MALFORMED_IMAGE);
  assert(out.count == -1);
  return 0;
}
