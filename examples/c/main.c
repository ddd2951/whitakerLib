#include <stdio.h>

#include <whitaker.h>

int main(void) {
  static WhitakerResult result;
  if (whitaker_init() != WHITAKER_OK)
    return 1;
  if (whitaker_analyze("amo", &result) != WHITAKER_OK)
    return 1;
  for (int i = 0; i < result.count; ++i) {
    const WhitakerMatch* match = &result.matches[i];
    printf("%s: %s — %s\n", match->orth, match->pos, match->meaning);
  }
  return 0;
}
