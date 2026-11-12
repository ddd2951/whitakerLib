#include <whitaker.h>
#include <assert.h>
#include <string.h>

int main(void) {
  WhitakerResult* result = whitaker_result_new();
  assert(result != NULL);
  assert(whitaker_analyze(result, "arma", 4) == WHITAKER_STATUS_OK);
  assert(whitaker_word_count(result) == 1 && whitaker_reading_count(result, 0) == 4);
  const WhitakerReadingDetail first = whitaker_detail(result, 0, 0);
  assert(first.dictionaryWordId == 4768 && strcmp(first.form.stemSpelling, "arm") == 0 &&
         strcmp(first.form.ending, "a") == 0);
  assert(first.form.grammar.part == WHITAKER_PART_N && first.form.grammar.caseOf == WHITAKER_CASE_NOM);
  assert(whitaker_append(result, "MMMM", 4) == WHITAKER_STATUS_OK);
  const WhitakerReadingDetail roman = whitaker_detail(result, 1, 0);
  assert(roman.dictionaryWordId == WHITAKER_NO_DICTIONARY_WORD && roman.dictionaryWord.numeralValue == 4000);
  const WhitakerText spelling = whitaker_spelling(result, 1);
  assert(spelling.size == 4 && memcmp(spelling.data, "MMMM", 4) == 0);
  assert(whitaker_detail(result, 2, 0).dictionaryWordId == 0 && whitaker_reading_count(result, 2) == 0);
  assert(whitaker_append(result, "aaaaaaaaaaaaaaaaaaaaaaaaa", 25) == WHITAKER_STATUS_INPUT_TOO_LONG);
  whitaker_clear(result);
  assert(whitaker_word_count(result) == 0);
  assert(strcmp(WHITAKER_VERSION, "0.4.0") == 0);
  whitaker_result_free(result);
  return 0;
}
