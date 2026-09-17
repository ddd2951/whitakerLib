#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "whitaker.h"

static WhitakerResult r;

static const WhitakerResult* look(const char* word) {
  assert(whitaker_analyze(word, &r) == WHITAKER_OK);
  return &r;
}

static bool same(const char* a, const char* b) { return strcmp(a, b) == 0; }

static bool hasInflection(const WhitakerResult* w, const char* inflection) {
  for (int i = 0; i < w->count; ++i)
    if (same(w->matches[i].inflection, inflection))
      return true;
  return false;
}

static bool hasAddon(const WhitakerMatch* m, WhitakerAddonKind kind) {
  for (uint8_t s = 0; s < m->addon_count; ++s)
    if (m->addons[s].kind == kind)
      return true;
  return false;
}

int main(void) {
  /* Before init: the image gate comes after argument checks. */
  assert(whitaker_analyze("amo", &r) == WHITAKER_NOT_INITIALIZED);
  assert(whitaker_analyze(NULL, &r) == WHITAKER_INVALID_ARGUMENT);
  assert(whitaker_init() == WHITAKER_OK);
  assert(whitaker_init() == WHITAKER_OK);

  /* Argument contract. On an error the result is not touched. */
  r.count = -1;
  assert(whitaker_analyze(NULL, &r) == WHITAKER_INVALID_ARGUMENT);
  assert(whitaker_analyze("amo", NULL) == WHITAKER_INVALID_ARGUMENT);
  assert(whitaker_analyze("abcdefghijklmnopqrstuvwxy", &r) ==
         WHITAKER_INPUT_TOO_LONG);
  assert(r.count == -1);
  assert(look("")->count == 0);
  assert(look("abcdefghijklmnopqrstuvwx")->count == 0);

  {
    const WhitakerResult* w = look("amo");
    assert(w->count == 1);
    assert(same(w->matches[0].orth, "am"));
    assert(same(w->matches[0].pos, "V"));
    assert(same(w->matches[0].inflection, "V C1 V1 PRES ACTIVE IND 1 S"));
    assert(w->matches[0].addon_count == 0);
  }
  assert(look("ama")->count == 4);

  /* A prefix is a strict fallback; the first record that answers wins. */
  {
    const WhitakerResult* w = look("succerealis");
    assert(w->count == 1);
    assert(same(w->matches[0].orth, "cereal"));
    assert(same(w->matches[0].pos, "N"));
    assert(w->matches[0].addon_count == 1);
    assert(w->matches[0].addons[0].kind == WHITAKER_ADDON_PREFIX);
    assert(same(w->matches[0].addons[0].spelling, "suc"));
  }

  /* A suffix replaces the grammar, and its stem keeps the fix on. */
  {
    const WhitakerResult* w = look("magalia");
    assert(w->count == 3);
    for (int i = 0; i < w->count; ++i) {
      assert(same(w->matches[i].orth, "magal"));
      assert(same(w->matches[i].pos, "ADJ"));
      assert(w->matches[i].addon_count == 1);
      assert(w->matches[i].addons[0].kind == WHITAKER_ADDON_SUFFIX);
      assert(same(w->matches[i].addons[0].spelling, "al"));
    }
  }

  /* Allowed_Stem asks about the stem the reading prints: `ic`, not `abic`. */
  assert(look("abic")->count == 0);

  /* A fix never adds to a word the plain search answered. */
  assert(look("praedico")->count == 2);

  /* Fix text is folded like a query: `v` in ADDONS.LAT matches `u`. */
  {
    const WhitakerResult* w = look("allambivam");
    assert(w->count == 1);
    assert(same(w->matches[0].orth, "allambiv"));
    assert(same(w->matches[0].pos, "ADJ"));
  }

  /* Enclitic: que augments a known word; ne and ve only an unanswered one.
     Try_Tackons keeps only the target part. */
  {
    const WhitakerResult* w = look("amoque");
    assert(w->count == 1);
    assert(same(w->matches[0].orth, "am"));
    assert(same(w->matches[0].pos, "V"));
    assert(w->matches[0].addon_count == 1);
    assert(w->matches[0].addons[0].kind == WHITAKER_ADDON_TACKON);
    assert(same(w->matches[0].addons[0].spelling, "que"));
    assert(strstr(w->matches[0].addons[0].meaning, "enclitic") != NULL);
  }
  {
    const WhitakerResult* w = look("mecum");
    assert(w->count == 2);
    for (int i = 0; i < w->count; ++i) {
      assert(same(w->matches[i].orth, "m"));
      assert(same(w->matches[i].pos, "PRON"));
    }
  }
  {
    const WhitakerResult* w = look("hicce");
    assert(w->count == 1);
    assert(same(w->matches[0].orth, "h"));
    assert(same(w->matches[0].pos, "PRON"));
  }
  {
    const WhitakerResult* w = look("atque");
    assert(w->count == 2);
    assert(same(w->matches[0].orth, "atque"));
    assert(same(w->matches[1].orth, "at"));
  }
  {
    const WhitakerResult* w = look("classemve");
    assert(w->count == 1);
    assert(same(w->matches[0].orth, "class"));
    assert(same(w->matches[0].inflection, "N D3 V3 ACC S F"));
  }
  {
    const WhitakerResult* w = look("sine");
    for (int i = 0; i < w->count; ++i)
      assert(!same(w->matches[i].orth, "si"));
    w = look("sive");
    for (int i = 0; i < w->count; ++i)
      assert(!same(w->matches[i].orth, "si"));
  }

  /* A packon keeps the record that supplied the second half of the word. */
  {
    const WhitakerResult* w = look("quicumque");
    bool found = false;
    for (int i = 0; i < w->count; ++i)
      if (w->matches[i].addon_count == 1 &&
          w->matches[i].addons[0].kind == WHITAKER_ADDON_PACKON) {
        assert(same(w->matches[i].addons[0].spelling, "cumque"));
        found = true;
      }
    assert(found);
  }

  /* The Qu block: a tickon on a qu-pronoun, PRON readings only, and the
     first record that answers wins. `cubi` is no qu-pronoun. */
  {
    const WhitakerResult* w = look("ecqui");
    assert(w->count > 0);
    bool nomPM = false, nomSM = false;
    for (int i = 0; i < w->count; ++i) {
      assert(same(w->matches[i].pos, "PRON"));
      assert(w->matches[i].addons[0].kind == WHITAKER_ADDON_TICKON);
      assert(same(w->matches[i].addons[0].spelling, "ec"));
      nomPM |= same(w->matches[i].inflection, "PRON D1 V0 NOM P M");
      nomSM |= same(w->matches[i].inflection, "PRON D1 V1 NOM S M");
    }
    assert(nomPM && nomSM);
    w = look("nescioquis");
    assert(w->count > 0);
    assert(same(w->matches[0].addons[0].spelling, "nescio"));
    w = look("necubi");
    for (int i = 0; i < w->count; ++i)
      assert(!hasAddon(&w->matches[i], WHITAKER_ADDON_TICKON));
  }

  /* A wildcard ending keeps the entry's conjugation and names its stem. */
  {
    const WhitakerResult* w = look("amavi");
    assert(w->count == 1);
    assert(same(w->matches[0].inflection, "V C1 V1 PERF ACTIVE IND 1 S"));
    assert(same(w->matches[0].orth, "amav"));
  }

  /* Several entries and all their inflections survive one lookup. */
  {
    const WhitakerResult* w = look("abacta");
    assert(w->count == 30);
    int adj = 0, vpar = 0;
    for (int i = 0; i < w->count; ++i) {
      assert(w->matches[i].orth && w->matches[i].meaning);
      assert(w->matches[i].pos && w->matches[i].inflection);
      assert(same(w->matches[i].orth, "abact"));
      if (same(w->matches[i].pos, "ADJ"))
        ++adj;
      if (same(w->matches[i].pos, "VPAR"))
        ++vpar;
    }
    assert(adj == 6 && vpar == 24);
  }

  /* Lookup is case-insensitive; orth keeps the dictionary's spelling. */
  {
    const int africa = look("africa")->count;
    assert(africa > 0 && look("Africa")->count == africa);
    assert(same(look("africa")->matches[0].orth, "Afric"));
    const int apollo = look("APOLLO")->count;
    assert(apollo > 0 && look("apollo")->count == apollo);
  }

  /* Deponent verbs have no active readings. */
  {
    const WhitakerResult* w = look("adfare");
    assert(w->count == 2);
    for (int i = 0; i < w->count; ++i)
      assert(strstr(w->matches[i].inflection, "ACTIVE") == NULL);
  }

  /* Only dic/duc/fac/fer stems accept the shortened imperative. */
  assert(hasInflection(look("adduc"), "V C3 V1 PRES ACTIVE IMP 2 S"));
  assert(hasInflection(look("abduc"), "V C3 V1 PRES ACTIVE IMP 2 S"));
  assert(hasInflection(look("addic"), "V C3 V1 PRES ACTIVE IMP 2 S"));
  assert(!hasInflection(look("abic"), "V C3 V1 PRES ACTIVE IMP 2 S"));

  /* Unknown words succeed with count 0. */
  assert(look("aqqqq")->count == 0);
  assert(look("yz")->count == 0);
  assert(look("4chan")->count == 0);

  /* Forms resolve across distant letter buckets. */
  assert(same(look("zebra")->matches[0].orth, "zebr"));
  assert(same(look("salus")->matches[0].orth, "salus"));
  assert(look("quis")->count > 0);

  /* j/i and v/u fold to the same key; unrelated letters stay distinct. */
  {
    const int iuppiter = look("iuppiter")->count;
    assert(iuppiter > 0 && look("Juppiter")->count == iuppiter);
    assert(same(look("iuppiter")->matches[0].orth, "Juppiter"));
    const int coniunx = look("coniunx")->count;
    assert(coniunx > 0 && look("conjunx")->count == coniunx);
    assert(same(look("coniunx")->matches[0].orth, "conjunx"));
    assert(same(look("unda")->matches[0].orth, "und"));
    assert(same(look("vir")->matches[0].orth, "vir"));
  }

  /* Synthesized esse: est uses the empty stem; sum/sunt stem 1, fuit stem 3. */
  {
    const WhitakerResult* w = look("est");
    int toBe = -1;
    for (int i = 0; i < w->count; ++i)
      if (same(w->matches[i].inflection, "V C5 V1 PRES ACTIVE IND 3 S"))
        toBe = i;
    assert(toBe >= 0);
    assert(same(w->matches[toBe].orth, ""));
    w = look("sum");
    assert(w->count == 1 && same(w->matches[0].orth, "s"));
    w = look("sunt");
    assert(w->count == 1 && same(w->matches[0].orth, "s"));
    w = look("fuit");
    assert(w->count == 1 && same(w->matches[0].orth, "fu"));
  }

  /* Unique records come before ordinary dictionary readings. */
  {
    const WhitakerResult* w = look("deus");
    assert(w->count == 2);
    assert(same(w->matches[0].orth, "deus"));
    assert(same(w->matches[0].inflection, "N D2 V1 VOC S M"));
    assert(same(w->matches[1].orth, "De"));
    w = look("vult");
    assert(w->count == 1);
    assert(same(w->matches[0].inflection, "V C6 V2 PRES ACTIVE IND 3 S"));
    w = look("mare");
    assert(same(w->matches[0].orth, "mare"));
    assert(same(w->matches[0].pos, "ADJ"));
  }

  /* An adverb is synthesized only when the dictionary has none. */
  {
    const WhitakerResult* w = look("corde");
    int adv = 0, adj = 0;
    for (int i = 0; i < w->count; ++i) {
      if (same(w->matches[i].pos, "ADV")) {
        ++adv;
        assert(same(w->matches[i].inflection, "ADV POS"));
        assert(same(w->matches[i].orth, "cord"));
      }
      if (same(w->matches[i].pos, "ADJ"))
        ++adj;
    }
    assert(adj >= 1 && adv == 1);
    w = look("male");
    int maleAdv = 0;
    for (int i = 0; i < w->count; ++i)
      if (same(w->matches[i].pos, "ADV")) {
        ++maleAdv;
        assert(same(w->matches[i].orth, "male"));
      }
    assert(maleAdv > 0);
  }

  /* A Roman numeral comes first, then the ordinary readings. The result owns
     its two strings. */
  {
    const WhitakerResult* w = look("i");
    assert(w->count > 1);
    assert(same(w->matches[0].pos, "NUM"));
    assert(same(w->matches[0].inflection, "NUM D2 V0 X X X CARD"));
    assert(same(w->matches[0].orth, "i"));
    assert(same(w->matches[0].meaning, " 1  as a ROMAN NUMERAL;"));
    assert(w->matches[0].orth >= w->text &&
           w->matches[0].orth < w->text + WHITAKER_TEXT_BYTES);
    w = look("mcmlxxxiv");
    assert(w->count == 1);
    assert(same(w->matches[0].meaning, " 1984  as a ROMAN NUMERAL;"));
    assert(same(look("vi")->matches[0].pos, "NUM"));
    w = look("ui");
    assert(w->count > 0);
    for (int i = 0; i < w->count; ++i)
      assert(!same(w->matches[i].pos, "NUM"));
    assert(look("iix")->count == 0);
  }

  /* qu/cu pronoun endings are keyed to the stem the input begins with. */
  {
    const WhitakerResult* w = look("cui");
    assert(hasInflection(w, "PRON D1 V0 DAT S X"));
    assert(!hasInflection(w, "PRON D1 V0 NOM P M"));
    w = look("qui");
    assert(hasInflection(w, "PRON D1 V0 NOM P M"));
    assert(!hasInflection(w, "PRON D1 V0 DAT S X"));
  }

  /* Golden counts and order for derived forms, as the Caesar comparison saw
     them. Readings that share a parse line are different dictionary entries. */
  {
    const WhitakerResult* w = look("quoque");
    assert(w->count == 87);
    assert(same(w->matches[0].orth, "quoque"));
    assert(same(w->matches[0].inflection, "ADV POS"));
    assert(w->matches[0].addon_count == 0);
    assert(same(w->matches[86].orth, "quo"));
    assert(same(w->matches[86].pos, "CONJ"));
    assert(w->matches[86].addons[0].kind == WHITAKER_ADDON_TACKON);
    int ablSN = 0;
    for (int i = 0; i < w->count; ++i)
      if (same(w->matches[i].inflection, "PRON D1 V0 ABL S N")) {
        for (int j = 0; j < i; ++j)
          if (same(w->matches[j].inflection, "PRON D1 V0 ABL S N"))
            assert(!same(w->matches[j].meaning, w->matches[i].meaning));
        ++ablSN;
      }
    assert(ablSN == 7);

    w = look("quicumque");
    assert(w->count == 13);
    assert(same(w->matches[0].inflection, "PRON D1 V0 DAT S X"));
    assert(same(w->matches[12].inflection, "PRON D1 V0 NOM P M"));

    /* quae + cum + que: the enclitic over the packon, both steps kept. */
    w = look("quaecumque");
    assert(w->count == 7);
    assert(w->matches[6].addon_count == 2);
    assert(w->matches[6].addons[0].kind == WHITAKER_ADDON_TACKON);
    assert(same(w->matches[6].addons[0].spelling, "que"));
    assert(w->matches[6].addons[1].kind == WHITAKER_ADDON_PACKON);
    assert(same(w->matches[6].addons[1].spelling, "cum"));
  }

  /* whitaker_result_copy rebases the strings a result owns; `=` would not. */
  {
    static WhitakerResult original, copy;
    assert(whitaker_analyze("mcmlxxxiv", &original) == WHITAKER_OK);
    whitaker_result_copy(&copy, &original);
    assert(whitaker_analyze("succerealis", &original) == WHITAKER_OK);
    assert(copy.count == 1);
    assert(same(copy.matches[0].orth, "mcmlxxxiv"));
    assert(same(copy.matches[0].meaning, " 1984  as a ROMAN NUMERAL;"));
    assert(copy.matches[0].orth >= copy.text &&
           copy.matches[0].orth < copy.text + WHITAKER_TEXT_BYTES);
    whitaker_result_copy(&copy, &original);
    assert(whitaker_analyze("amo", &original) == WHITAKER_OK);
    assert(copy.count == 1);
    assert(same(copy.matches[0].orth, "cereal"));
    assert(copy.matches[0].orth >= copy.text &&
           copy.matches[0].orth < copy.text + WHITAKER_TEXT_BYTES);
    assert(same(copy.matches[0].addons[0].spelling, "suc"));
    whitaker_result_copy(&copy, &copy);
    whitaker_result_copy(NULL, &copy);
    whitaker_result_copy(&copy, NULL);
    assert(same(copy.matches[0].orth, "cereal"));
  }

  /* A result outlives later calls: its strings are its own or static. */
  {
    static WhitakerResult kept;
    assert(whitaker_analyze("mcmlxxxiv", &kept) == WHITAKER_OK);
    assert(look("succerealis")->count == 1);
    assert(look("amo")->count == 1);
    assert(same(kept.matches[0].meaning, " 1984  as a ROMAN NUMERAL;"));
    assert(same(kept.matches[0].orth, "mcmlxxxiv"));
  }

  return 0;
}
