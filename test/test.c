/* The API contract in whitaker.h. What the library answers for real words is
   the examples' job (examples/expected/); this checks what they don't show. */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "whitaker.h"

static_assert(WHITAKER_MAX_WORD_LENGTH == 24, "the long words below");

static WhitakerResult r;

static const WhitakerResult* look(const char* word) {
  assert(whitaker_analyze(word, &r) == WHITAKER_OK);
  return &r;
}

static bool same(const char* a, const char* b) { return strcmp(a, b) == 0; }

static bool inside(const char* s, const WhitakerResult* result) {
  return (uintptr_t)s - (uintptr_t)result->text < WHITAKER_TEXT_BYTES;
}

int main(void) {
  /* No init needed; calling it anyway is harmless. */
  assert(look("amo")->count > 0);
  assert(whitaker_init() == WHITAKER_OK);
  assert(whitaker_init() == WHITAKER_OK);

  /* Arguments. On an error the result is not touched. */
  r.count = -1;
  assert(whitaker_analyze(NULL, &r) == WHITAKER_INVALID_ARGUMENT);
  assert(whitaker_analyze("amo", NULL) == WHITAKER_INVALID_ARGUMENT);
  assert(whitaker_analyze("abcdefghijklmnopqrstuvwxy", &r) == WHITAKER_INPUT_TOO_LONG);
  assert(r.count == -1);

  /* Anything else is OK, with count 0 when nothing reads. */
  assert(look("")->count == 0);
  assert(look("abcdefghijklmnopqrstuvwx")->count == 0);
  assert(look("4chan")->count == 0);

  /* Case is insignificant; j folds with i and v with u. */
  {
    const int count = look("iuppiter")->count;
    assert(count > 0);
    assert(look("IUPPITER")->count == count);
    assert(look("Juppiter")->count == count);
    assert(look("ivppiter")->count == count);
  }

  /* Every reading has its strings, and pos names its grammar's part. */
  {
    const char* words[] = {"amo", "amoque", "succerealis", "est", "XIV"};
    for (size_t k = 0; k < sizeof words / sizeof *words; ++k) {
      const WhitakerResult* w = look(words[k]);
      assert(w->count > 0);
      for (int i = 0; i < w->count; ++i) {
        const WhitakerMatch* m = &w->matches[i];
        assert(m->orth && m->meaning && m->pos);
        assert(same(whitaker_name(WHITAKER_FIELD_PART, m->grammar.part), m->pos));
        for (uint8_t s = 0; s < m->addon_count; ++s)
          assert(whitaker_addon_spelling(m->addons[s].id) && whitaker_addon_meaning(m->addons[s].id));
      }
    }
  }

  /* whitaker_describe works like snprintf. */
  {
    const WhitakerGrammar* g = &look("amo")->matches[0].grammar;
    char line[64], small[4];
    const size_t length = whitaker_describe(g, line, sizeof line);
    assert(same(line, "V C1 V1 PRES ACTIVE IND 1 S"));
    assert(length == strlen(line));
    assert(whitaker_describe(g, small, sizeof small) == length);
    assert(same(small, "V C"));
    assert(whitaker_describe(g, NULL, 0) == length);
    assert(whitaker_describe(NULL, line, sizeof line) == 0);
  }

  /* Names, and NULL for what the library doesn't have. */
  assert(same(whitaker_name(WHITAKER_FIELD_CASE, WHITAKER_CASE_NOM), "NOM"));
  assert(whitaker_name(WHITAKER_FIELD_CASE, 200) == NULL);
  assert(whitaker_addon_spelling(UINT16_MAX) == NULL);
  assert(whitaker_addon_meaning(UINT16_MAX) == NULL);

  /* A result owns the strings it prints, and keeps them through later calls.
     whitaker_result_copy points the copy's strings into the copy. */
  {
    static WhitakerResult kept, copy;
    assert(whitaker_analyze("mcmlxxxiv", &kept) == WHITAKER_OK);
    assert(inside(kept.matches[0].orth, &kept));
    assert(inside(kept.matches[0].meaning, &kept));
    whitaker_result_copy(&copy, &kept);
    assert(look("succerealis")->count > 0);
    assert(same(kept.matches[0].orth, "mcmlxxxiv"));
    assert(same(kept.matches[0].meaning, " 1984  as a ROMAN NUMERAL;"));

    memset(&kept, 0, sizeof kept);
    assert(same(copy.matches[0].orth, "mcmlxxxiv"));
    assert(inside(copy.matches[0].orth, &copy));
    assert(inside(copy.matches[0].meaning, &copy));

    /* The same object twice, or a null, does nothing. */
    whitaker_result_copy(&copy, &copy);
    whitaker_result_copy(NULL, &copy);
    whitaker_result_copy(&copy, NULL);
    assert(same(copy.matches[0].orth, "mcmlxxxiv"));
  }

  /* The word may live in the result it is analyzed into. */
  {
    static WhitakerResult own, apart;
    strcpy(own.text, "eo");
    assert(whitaker_analyze(own.text, &own) == WHITAKER_OK);
    assert(whitaker_analyze("eo", &apart) == WHITAKER_OK);
    assert(own.count == apart.count && own.count > 1);
    for (int i = 0; i < own.count; ++i)
      assert(same(own.matches[i].orth, apart.matches[i].orth));
  }
  return 0;
}
