# How the answers were checked

The tests in `test/` prove single words. The library's answers as a whole were
checked against the original Ada WORDS program over a real text. The
comparison tool is development tooling and is not part of this library; this
note records what was compared and what came out.

## The text

Caesar, *De bello Gallico*, from the Perseus Digital Library
(`caesar-gallic-war.perseus-lat2.xml`, SHA-256 `15ec3ef2…17ca1568`), split into
one ASCII Latin token per line: 51,300 occurrences of 10,940 distinct
spellings (token file SHA-256 `a15abace…5fbab2f24`).

## The comparison

Both programs were run for every occurrence. Each analysis was reduced to its
part of speech and inflection description, with only WORDS' display
conventions removed: age and frequency labels, deponent-voice presentation,
the `V 3 4` rendering, the artificial `PRON 1 X` variant, and a literal `0`
for no person. Stems, meanings, duplicate counts, and result order were not
compared.

| Occurrences | Category |
|------------:|----------|
| 49,885 | exact — the same non-empty analysis set |
| 384 | both answered, sharing at least one analysis |
| 373 | neither answered |
| 295 | Ada only |
| 347 | library only |
| 16 | both answered, sharing nothing |

The 1,042 differing occurrences come almost entirely from four causes. WORDS
labels the first three in its output, which makes most of the attribution
direct rather than inferred.

- **Syncope** (501). WORDS' `Perform_Syncope` (`words_engine-parse.adb:498`)
  reads a contracted perfect as its full form: `comparare` also as
  `comparavere`, `venisset` as `venivisset`, `petierunt` as `petiverunt`. The
  library has no such rule.
- **Two-word guesses** (139). WORDS splits an unknown word in two and answers
  the halves (`volu+senus`, `lucter+ius`), marking the result "if not obvious,
  probably incorrect". The library does not guess.
- **Tricks** (about 55). WORDS' `Try_Tricks` substitutes letters inside an
  unknown word: `d/t`, `ci/ti`, `ae/e`, `m/n` before a packon. The library
  does not.
- **One-letter words** (344). WORDS answers nothing for `a`, `c`, `e`, `m`;
  the library answers them from the dictionary.

Both programs read the same `DICTLINE.GEN`; no difference comes from the
dictionary. The other 373 occurrences outside `exact` were answered by
neither program.

## The Aeneid transcript

Upstream `test/10_aeneid/expected.txt` (revision `e9031c9253a6`) is genuine
WORDS output over a sample of the Aeneid and was the first authority used. It
records answers without the words that produced them, and reflects a newer
dictionary than the pinned one, so the Caesar comparison above replaced it.
