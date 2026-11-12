// INFO: The C binding: every reading of each word given, with its form, part of speech and meaning.
#include <whitaker.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char** argv) {
  WhitakerResult* result = whitaker_result_new();
  if (result == NULL)
    return 1;
  for (int i = 1; i < argc; ++i) {
    if (whitaker_analyze(result, argv[i], strlen(argv[i])) != WHITAKER_STATUS_OK)
      continue;
    const size_t count = whitaker_reading_count(result, 0);
    printf("%s: %zu readings\n", argv[i], count);
    for (size_t r = 0; r < count; ++r) {
      const WhitakerReadingDetail detail = whitaker_detail(result, 0, r);
      printf("  %s.%s  %s  %s\n", detail.form.stemSpelling, detail.form.ending, detail.partName, detail.meaning);
    }
  }
  whitaker_result_free(result);
  return 0;
}
