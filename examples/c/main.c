/* Analyzes the words given as arguments, or a text on stdin, one word or more
   per line, and prints every reading. For a text, a summary goes to stderr:

     whitaker-c-example amo amoque
     whitaker-c-example < text.words > /dev/null
*/
#include <stdio.h>
#include <time.h>

#include <whitaker.h>

static double milliseconds(void) {
  struct timespec now;
  timespec_get(&now, TIME_UTC);
  return (double)now.tv_sec * 1e3 + (double)now.tv_nsec / 1e6;
}

static const char* addonKind(uint8_t kind) {
  switch (kind) {
  case WHITAKER_ADDON_PREFIX:
    return "prefix";
  case WHITAKER_ADDON_SUFFIX:
    return "suffix";
  case WHITAKER_ADDON_TACKON:
    return "tackon";
  case WHITAKER_ADDON_PACKON:
    return "packon";
  case WHITAKER_ADDON_TICKON:
    return "tickon";
  default:
    return "addon";
  }
}

static long readings;
static double analyzing;

static void print(const char* word) {
  /* About 34 KiB, so not on the stack. */
  static WhitakerResult result;
  const double start = milliseconds();
  const WhitakerStatus status = whitaker_analyze(word, &result);
  analyzing += milliseconds() - start;

  printf("%s\n", word);
  if (status == WHITAKER_INPUT_TOO_LONG) {
    printf("  longer than %d letters\n", WHITAKER_MAX_WORD_LENGTH);
    return;
  }
  if (status != WHITAKER_OK) {
    printf("  error %d\n", (int)status);
    return;
  }
  if (result.count == 0)
    printf("  no reading\n");
  readings += result.count;

  for (int i = 0; i < result.count; ++i) {
    const WhitakerMatch* match = &result.matches[i];
    char grammar[64];
    whitaker_describe(&match->grammar, grammar, sizeof grammar);
    printf("  %s | %s", match->orth, grammar);
    /* The add-ons around the stem, the outermost first. */
    for (uint8_t step = 0; step < match->addon_count; ++step)
      printf(" | %s %s", addonKind(match->addons[step].kind), whitaker_addon_spelling(match->addons[step].id));
    printf("\n    %s\n", match->meaning);
  }
}

int main(int argc, char** argv) {
  for (int i = 1; i < argc; ++i)
    print(argv[i]);
  if (argc > 1)
    return 0;

  long words = 0;
  char word[256];
  while (scanf("%255s", word) == 1) {
    print(word);
    ++words;
  }
  fprintf(stderr, "%ld words, %ld readings, %.1f ms analyzing\n", words, readings, analyzing);
  return 0;
}
