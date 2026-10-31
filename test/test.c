// INFO: Public API contracts; word readings are checked by examples/expected/.
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "whitaker.h"

static_assert(WHITAKER_MAX_WORD_LENGTH == 24, "the long words below");

static WhitakerResult* r;

static bool same(const char* a, const char* b) { return strcmp(a, b) == 0; }

static bool sameSteps(const WhitakerRecord* a, const WhitakerRecord* b) {
  if (a->addon_count != b->addon_count)
    return false;
  for (uint8_t i = 0; i < a->addon_count; ++i)
    if (a->addons[i].id != b->addons[i].id || a->addons[i].kind != b->addons[i].kind)
      return false;
  return true;
}

// INFO: Contiguous forms and distinct neighbouring record blocks.
static void checkWord(const WhitakerResult* result, size_t w) {
  const WhitakerWord* word = &result->words[w];
  size_t next = word->record_count ? result->records[word->first_record].first_form : 0;
  for (size_t k = 0; k < word->record_count; ++k) {
    const WhitakerRecord* record = &result->records[word->first_record + k];
    assert(record->form_count > 0 && record->first_form == next);
    next += record->form_count;
    assert(record->meaning && record->pos);
    assert((record->addon_count == 0) == (record->addons == NULL));
    for (uint8_t s = 0; s < record->addon_count; ++s)
      assert(whitaker_addon_spelling(record->addons[s].id) && whitaker_addon_meaning(record->addons[s].id));
    for (size_t f = 0; f < record->form_count; ++f) {
      const WhitakerForm* form = &result->forms[record->first_form + f];
      assert(form->orth && form->ending);
      assert(same(whitaker_name(WHITAKER_FIELD_PART, form->grammar.part), record->pos));
    }
    if (k > 0) {
      const WhitakerRecord* before = record - 1;
      assert(before->entry.id != record->entry.id || !same(before->pos, record->pos) ||
             before->route != record->route || before->spelling_rule != record->spelling_rule ||
             !sameSteps(before, record));
    }
  }
}

static size_t formsOf(const WhitakerResult* result, size_t w) {
  size_t forms = 0;
  for (size_t k = 0; k < result->words[w].record_count; ++k)
    forms += result->records[result->words[w].first_record + k].form_count;
  return forms;
}

static const WhitakerResult* look(const char* word) {
  assert(whitaker_analyze(word, r) == WHITAKER_OK);
  assert(r->word_count == 1 && r->words[0].first_record == 0 && r->words[0].record_count == r->record_count);
  assert(formsOf(r, 0) == r->form_count);
  checkWord(r, 0);
  return r;
}

int main(void) {
  r = whitaker_result_new();
  assert(r && r->word_count == 0 && r->record_count == 0 && r->form_count == 0);

  // INFO: Invalid arguments and oversized input leave the result unchanged.
  assert(look("amo")->form_count > 0);
  const size_t amo = r->form_count;
  assert(whitaker_analyze(NULL, r) == WHITAKER_INVALID_ARGUMENT);
  assert(whitaker_analyze("amo", NULL) == WHITAKER_INVALID_ARGUMENT);
  assert(whitaker_analyze_append(NULL, r) == WHITAKER_INVALID_ARGUMENT);
  assert(whitaker_analyze("abcdefghijklmnopqrstuvwxy", r) == WHITAKER_INPUT_TOO_LONG);
  assert(whitaker_analyze_append("abcdefghijklmnopqrstuvwxy", r) == WHITAKER_INPUT_TOO_LONG);
  assert(r->form_count == amo && r->word_count == 1);
  static WhitakerResult notMade;
  assert(whitaker_analyze("amo", &notMade) == WHITAKER_INVALID_ARGUMENT);

  // INFO: Empty and unknown spellings return a word with no records.
  assert(look("")->record_count == 0);
  assert(look("abcdefghijklmnopqrstuvwx")->record_count == 0);
  assert(look("4chan")->record_count == 0);

  // INFO: Dictionary lookup folds case, j/i and v/u.
  {
    const size_t forms = look("iuppiter")->form_count;
    assert(forms > 0);
    assert(look("IUPPITER")->form_count == forms);
    assert(look("Juppiter")->form_count == forms);
    assert(look("ivppiter")->form_count == forms);
  }

  // INFO: Records and forms for add-ons, numerals and ambiguous words.
  {
    const char* words[] = {"amo", "amoque", "succerealis", "est", "XIV", "arma", "virumque"};
    for (size_t k = 0; k < sizeof words / sizeof *words; ++k)
      assert(look(words[k])->record_count > 0);
    assert(look("arma")->record_count == 2 && r->records[0].form_count == 3 && r->records[1].form_count == 1);
  }

  // INFO: whitaker_describe follows snprintf semantics.
  {
    const WhitakerGrammar* g = &look("amo")->forms[0].grammar;
    char line[64], small[4];
    const size_t length = whitaker_describe(g, line, sizeof line);
    assert(same(line, "V C1 V1 PRES ACTIVE IND 1 S"));
    assert(length == strlen(line));
    assert(whitaker_describe(g, small, sizeof small) == length);
    assert(same(small, "V C"));
    assert(whitaker_describe(g, NULL, 0) == length);
    assert(whitaker_describe(NULL, line, sizeof line) == 0);
  }

  // INFO: Names are NULL for invalid values and ids.
  assert(same(whitaker_name(WHITAKER_FIELD_CASE, WHITAKER_CASE_NOM), "NOM"));
  assert(whitaker_name(WHITAKER_FIELD_CASE, 200) == NULL);
  assert(whitaker_addon_spelling(UINT16_MAX) == NULL);
  assert(whitaker_addon_meaning(UINT16_MAX) == NULL);

  // INFO: Appended words match separate lookups.
  {
    const char* words[] = {"arma", "virumque", "cano"};
    WhitakerResult* line = whitaker_result_new();
    for (size_t w = 0; w < 3; ++w)
      assert(whitaker_analyze_append(words[w], line) == WHITAKER_OK);
    assert(line->word_count == 3);
    size_t firstRecord = 0;
    for (size_t w = 0; w < 3; ++w) {
      const WhitakerWord* word = &line->words[w];
      assert(word->first_record == firstRecord);
      checkWord(line, w);
      look(words[w]);
      assert(word->record_count == r->record_count && word->trimmed == r->words[0].trimmed);
      for (size_t k = 0; k < word->record_count; ++k) {
        const WhitakerRecord* a = &line->records[word->first_record + k];
        const WhitakerRecord* b = &r->records[k];
        assert(a->entry.id == b->entry.id && a->form_count == b->form_count);
        for (size_t f = 0; f < a->form_count; ++f) {
          assert(same(line->forms[a->first_form + f].orth, r->forms[b->first_form + f].orth));
          assert(same(line->forms[a->first_form + f].ending, r->forms[b->first_form + f].ending));
        }
      }
      firstRecord += word->record_count;
    }
    assert(firstRecord == line->record_count);

    // NOTE: Strings stay put while the result grows.
    const char* const arma = line->forms[0].orth;
    for (int i = 0; i < 500; ++i)
      assert(whitaker_analyze_append("succerealis", line) == WHITAKER_OK);
    assert(line->forms[0].orth == arma && same(arma, "arm"));

    whitaker_result_clear(line);
    assert(line->word_count == 0 && line->record_count == 0 && line->form_count == 0);
    assert(whitaker_analyze_append("cano", line) == WHITAKER_OK && line->word_count == 1);
    whitaker_result_free(line);
  }

  // NOTE: Cloned strings, including Roman numeral meanings, outlive the original.
  {
    WhitakerResult* kept = whitaker_result_new();
    WhitakerResult* empty = whitaker_result_clone(kept);
    assert(empty && empty->word_count == 0 && empty->record_count == 0 && empty->form_count == 0);
    whitaker_result_free(empty);
    assert(whitaker_analyze("mcmlxxxiv", kept) == WHITAKER_OK);
    WhitakerResult* copy = whitaker_result_clone(kept);
    assert(copy && copy->form_count == kept->form_count && copy->word_count == 1);
    assert(copy->forms[0].orth != kept->forms[0].orth);
    whitaker_result_free(kept);
    assert(same(copy->forms[0].orth, "mcmlxxxiv"));
    assert(same(copy->records[0].meaning, " 1984  as a ROMAN NUMERAL;"));
    checkWord(copy, 0);
    whitaker_result_free(copy);
    assert(whitaker_result_clone(NULL) == NULL);
  }

  // NOTE: Input may point into the result being replaced.
  {
    WhitakerResult* apart = whitaker_result_new();
    assert(whitaker_analyze("mcmlxxxiv", apart) == WHITAKER_OK);
    assert(whitaker_analyze("mcmlxxxiv", r) == WHITAKER_OK && same(r->forms[0].orth, "mcmlxxxiv"));
    assert(whitaker_analyze(r->forms[0].orth, r) == WHITAKER_OK);
    assert(r->form_count == apart->form_count && r->form_count > 0);
    for (size_t i = 0; i < r->form_count; ++i)
      assert(same(r->forms[i].orth, apart->forms[i].orth));
    whitaker_result_free(apart);
  }

  // INFO: Clearing and freeing NULL are harmless.
  whitaker_result_clear(NULL);
  whitaker_result_free(NULL);
  whitaker_result_free(r);
  return 0;
}
