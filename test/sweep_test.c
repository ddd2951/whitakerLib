#include <whitaker.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static WhitakerResult r;
static long readings, failures;

static bool same(const char* a, const char* b) { return strcmp(a, b) == 0; }

static void fail(const char* word, const char* what) {
  if (failures++ < 10)
    fprintf(stderr, "%s: %s\n", word, what);
}

static bool partsMatch(const WhitakerGrammar* g, const WhitakerEntry* e) {
  if (e->part == g->part)
    return true;
  switch (g->part) {
  case WHITAKER_PART_VPAR:
  case WHITAKER_PART_SUPINE:
    return e->part == WHITAKER_PART_V;
  case WHITAKER_PART_PRON:
    return e->part == WHITAKER_PART_PACK;
  case WHITAKER_PART_ADV:
    return true;
  default:
    return false;
  }
}

static void sweep(const char* word) {
  if (strlen(word) > WHITAKER_MAX_WORD_LENGTH ||
      whitaker_analyze(word, &r) != WHITAKER_OK)
    return;
  char line[128];
  for (int i = 0; i < r.count; ++i) {
    const WhitakerMatch* m = &r.matches[i];
    ++readings;
    if (!m->orth || !m->meaning || !m->pos ||
        m->addon_count > WHITAKER_MAX_ADDON_STEPS) {
      fail(word, "a field is unset");
      continue;
    }
    for (int a = 0; a < m->addon_count; ++a)
      if (!whitaker_addon_spelling(m->addons[a].id) ||
          !whitaker_addon_meaning(m->addons[a].id))
        fail(word, "an addon step is unset");
    const size_t length = whitaker_describe(&m->grammar, line, sizeof line);
    if (length == 0 || length >= sizeof line)
      fail(word, "grammar does not render a line");
    if (strstr(m->meaning, "ROMAN NUMERAL"))
      continue;
    const WhitakerGrammar* g = &m->grammar;
    const WhitakerEntry* e = &m->entry;
    if (e->part == WHITAKER_PART_X)
      fail(word, "entry has no part");
    if (m->addon_count != 0)
      continue;
    if (!partsMatch(g, e))
      fail(word, "entry part does not fit the reading");
    else if (g->part != WHITAKER_PART_ADV && g->part != WHITAKER_PART_PREP &&
             g->part != WHITAKER_PART_CONJ &&
             g->part != WHITAKER_PART_INTERJ &&
             (g->which != e->which || g->variant != e->variant))
      fail(word, "entry number is not the reading's");
    else if (g->part == WHITAKER_PART_N && g->gender != e->gender)
      fail(word, "entry gender is not the noun's");
  }
}

static void sweepAffixed(const char* stem) {
  static const char* const prefixes[] = {"in", "con", "re", "ab", "de",
                                         "ex", "per", "sub", "prae"};
  static const char* const tails[] = {"que", "ne",  "ve",   "cumque", "met",
                                      "te",  "a",   "us",   "is",     "em",
                                      "ibus", "atus", "ando", "ens",    "orum",
                                      "tur"};
  char word[64];
  sweep(stem);
  for (size_t i = 0; i < sizeof prefixes / sizeof *prefixes; ++i) {
    snprintf(word, sizeof word, "%s%s", prefixes[i], stem);
    sweep(word);
  }
  for (size_t i = 0; i < sizeof tails / sizeof *tails; ++i) {
    snprintf(word, sizeof word, "%s%s", stem, tails[i]);
    sweep(word);
  }
}

static char stems[200000][32];

static int byText(const void* a, const void* b) { return strcmp(a, b); }

int main(int argc, char** argv) {
  if (argc != 2 || whitaker_init() != WHITAKER_OK)
    return 2;
  FILE* dictline = fopen(argv[1], "r");
  if (!dictline)
    return 2;
  char text[512];
  size_t count = 0;
  while (fgets(text, sizeof text, dictline)) {
    // NOTE: DICTLINE's four stem columns occupy the first 76 characters.
    text[76] = '\0';
    for (char* token = strtok(text, " "); token; token = strtok(NULL, " ")) {
      bool letters = token[0] != '\0' && strlen(token) < sizeof *stems &&
                     !same(token, "zzz");
      for (char* c = token; *c && letters; ++c) {
        if (*c >= 'A' && *c <= 'Z')
          *c = (char)(*c - 'A' + 'a');
        letters = *c >= 'a' && *c <= 'z';
      }
      if (letters && count < sizeof stems / sizeof *stems)
        strcpy(stems[count++], token);
    }
  }
  fclose(dictline);
  qsort(stems, count, sizeof *stems, byText);
  long distinct = 0;
  for (size_t i = 0; i < count; ++i)
    if (i == 0 || !same(stems[i], stems[i - 1])) {
      sweepAffixed(stems[i]);
      ++distinct;
    }
  printf("%ld stems, %ld readings, %ld failures\n", distinct, readings,
         failures);
  return failures == 0 && readings > 0 ? 0 : 1;
}
