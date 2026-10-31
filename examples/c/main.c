// INFO: Analyze arguments or whitespace-separated words from stdin; print a summary to stderr for stdin.
//
//   whitaker-c-example amo amoque
//   whitaker-c-example < text.words > /dev/null
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

static WhitakerResult* result;
static long readings;
static double analyzing;

static void print(const char* word) {
  const double start = milliseconds();
  const WhitakerStatus status = whitaker_analyze(word, result);
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
  if (result->form_count == 0)
    printf("  no reading\n");
  readings += (long)result->form_count;

  // INFO: Each record's forms, then its add-ons (outermost first) and its meaning.
  for (size_t r = 0; r < result->record_count; ++r) {
    const WhitakerRecord* record = &result->records[r];
    for (size_t f = 0; f < record->form_count; ++f) {
      const WhitakerForm* form = &result->forms[record->first_form + f];
      char grammar[64];
      whitaker_describe(&form->grammar, grammar, sizeof grammar);
      printf("  %s | %s", form->orth, grammar);
      for (uint8_t step = 0; step < record->addon_count; ++step)
        printf(" | %s %s", addonKind(record->addons[step].kind), whitaker_addon_spelling(record->addons[step].id));
      printf("\n    %s\n", record->meaning);
    }
  }
}

int main(int argc, char** argv) {
  // NOTE: Reuse one result to retain capacity between words.
  result = whitaker_result_new();
  if (result == NULL)
    return 1;
  for (int i = 1; i < argc; ++i)
    print(argv[i]);
  if (argc > 1) {
    whitaker_result_free(result);
    return 0;
  }

  long words = 0;
  char word[256];
  while (scanf("%255s", word) == 1) {
    print(word);
    ++words;
  }
  fprintf(stderr, "%ld words, %ld readings, %.1f ms analyzing\n", words, readings, analyzing);
  whitaker_result_free(result);
  return 0;
}
