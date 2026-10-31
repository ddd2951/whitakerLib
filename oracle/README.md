# Lib Oracle

While working on whitakerLib I kept running into two issues:

- It's very easy to change one thing and end up affecting words you weren't expecting.
- The original Ada WORDS simply takes too long to rerun over a large collection of books after every small change.

So I made this oracle to try and fix both.

The idea is simple. We capture the output of Ada WORDS using the books you give it. Since the same word can appear
thousands of times, we only store the answer once to save space.
We then run whitakerLib against those stored answers without having to rerun Ada.

An earlier warm-cache comparison of the 102 books took about 26 seconds
(median of three). That is historical development timing, not a release
benchmark. No comparable Ada timing was recorded.

I've mostly increased the accuracy by slowly looking at the biggest mismatches and figuring out what caused them.
We have now reached the point where every mismatch in the pinned collection of books has a known cause.
We also ended up finding 20 bugs in the original WORDS along the way, some of which would crash the program.
The comparisons use a patched WORDS build. All 19 completed fixes and their
regression tests are available in my [combined_fixes branch][combined-fixes].

The [book list](../data/booklist.md) records the texts and per-book results.
The saved answers record the local build as `5dd8ca5`; its Ada source matches
the published revision linked below.

## Run it

From the library root, use `oracle/oracle` (shown as `oracle` below):

```sh
oracle                  # compare the library against the captures, write the report
oracle -r               # show the latest report
oracle -s amoque        # one word: every split and pairing
oracle -p               # probe: change each field, check the comparison notices
oracle -c               # capture books that are missing or stale under stored/
oracle -c -f            # recapture everything
oracle -a FILE|FOLDER   # add books to the corpus (then capture only those)
oracle -b               # list books: captured or not, against which WORDS
```
Captures are kept under `stored/`, meaning Ada only needs to run again when books are added or something has changed.
The command builds the oracle and the selected library automatically. The
oracle needs CMake 3.28 and GCC 16 with C++26 reflection (`-freflection`).

To build it directly, with WORDS' data already available:
```sh
cmake -S oracle -B oracle/build/self -DADA_DATA_DIR=/path/to/words/data
cmake --build oracle/build/self --target lines
```
Use:
- `--words BIN --data DIR` to select WORDS
- `--lib DIR` for another library tree
- `--store DIR` for a separate capture and report store

Register plain ASCII words, one per line;
`oracle -a -x FILE` registers a fixture outside the book totals. Missing, stale or incomplete captures stop a comparison.

## How the oracle compares whitakerLib and the original WORDS

WORDS and whitakerLib don't return their answers in the same way.

WORDS prints its answers for an 80-column terminal, while whitakerLib returns typed fields such as enums, numbers and strings.

WORDS groups readings under dictionary entries; whitakerLib returns records with their forms.

The oracle uses WORDS' own printing rules to show the library's matches as WORDS would have printed them.

Afterward it lines up the results and checks:
- Every line from WORDS has a corresponding line from the library, and vice versa.
- The readings appear in the same order.
- Every value WORDS shows corresponds to a library field with the same value.
- Readings sharing a dictionary entry agree on its fields.

For dictionary information, the oracle also reads WORDS' own records using the library's entry IDs, rather than
trusting the library to provide both sides of the comparison. WORDS prints one dictionary form line for records
with the same dictionary form, leaving out repeated information. The library gives each record its own codes.
Even when WORDS leaves out a dictionary form line, the oracle checks that record directly against WORDS' own records.

Things like spacing, how WORDS prints a value, or meanings cut short to fit the terminal don't count as
differences if the underlying values agree.

We count the results as:
- **Gap:** WORDS returns something the library doesn't support.
- **Difference:** Something else doesn't match, and we need to figure out why.

As an example look at `arma`:
WORDS prints three noun readings under `armum`, then the imperative of `armo`.
The library returns two records containing four forms:

| WORDS reading | Library record | Form grammar |
| --- | --- | --- |
| arm.a N 2 2 NOM P N | 4769, armum | NOM P N |
| arm.a N 2 2 VOC P N | 4769, armum | VOC P N |
| arm.a N 2 2 ACC P N | 4769, armum | ACC P N |
| arm.a V 1 1 PRES ACTIVE IMP 2 S | 4770, armo | PRES ACTIVE IMP 2 S |

`oracle -s arma` shows all eight printed lines paired: four readings,
two dictionary forms and two meanings, with every field equal.

The layout follows WORDS' `Put_Parse_Details` and `Put_Dictionary_Form`,
independently of the captured answer. Lines are paired in order, then unmatched
lines of the same kind are compared field by field. Nothing left over is ignored.

The printing rules are deliberately narrow:
- Fold case and j/i, v/u where WORDS does.
- Strip a meaning of leading continuation bars and compare its first 79 characters.
- Allow an omitted person 0 and WORDS' blank deponent voice; hidden values are not compared.
- Check constructed numeral meanings against the numeral's value and sort.

Each rule is counted in the report; changes need review against the Ada routine.
The rule table is in [lines.hpp](src/lines.hpp), field comparisons in
[compare.cpp](src/compare.cpp), and layout in [library_lines.cpp](src/library_lines.cpp).

## Validating the oracle

I've set up a few checks to make sure the oracle itself isn't missing anything:
- Every field in the library has to be accounted for.
- Change each field on purpose and check that the oracle catches it.
- Rebuild WORDS' printed forms from its own dictionary records.
- Make sure every line from both sides is accounted for.
- Check that readings sharing a dictionary entry agree on its fields.
- When the oracle changes, show which words get different results.

Reflection checks the field table in [probe.cpp](src/probe.cpp) against
`WhitakerRecord` and `WhitakerForm`. The probe mutates every mapped field, then
each record or form alone, to catch fields checked only at the start of an
answer. Fields WORDS doesn't show must leave the comparison unchanged. The independent dictionary
reader refuses mismatched source data; every run also rebuilds WORDS' numbered
dictionary forms and checks them against the captures.

## Bugs found in WORDS

Something I didn't originally expect was how many bugs the comparisons would uncover in WORDS itself.

So far, I've found 20. Some give incorrect readings, others silently drop answers, and a few can crash WORDS entirely.

If WORDS has a bug, I don't really want to reproduce that bug in whitakerLib just to get a matching result.

Five fixes have been submitted to the upstream project. All 19 completed fixes
and their regression tests are available in the [combined_fixes branch][combined-fixes].
For a fixed source revision, pin to
[`da6b35f`](https://github.com/ddd2951/whitakers-words/commit/da6b35fc862f1019720993dcddf7fd9d62ecc080).
Its Ada source is identical to the local build used for the saved captures;
the captures retain their original source revision and build details.

The [WORDS fix notes](WORDS_FIXES.md) give a short overview with regression
examples and links to the tests. The cases use WORDS' existing five test folders.

The first five entries below follow upstream pull-request order. The combined
branch includes fixes #1–#19; #20 remains open and is excluded.

| # | Finding | Cause | Patch |
| --- | --- | --- | --- |
| 1 | Syncope state | Failed syncope attempts overwrite dictionary candidates. | [#151](https://github.com/mk270/whitakers-words/pull/151) |
| 2 | Enclitic syncope | Syncope is tried on the whole word instead of its base. | [#152](https://github.com/mk270/whitakers-words/pull/152) |
| 3 | Comment reader | A short line is tested using stale buffer text. | [#153](https://github.com/mk270/whitakers-words/pull/153) |
| 4 | Slur and tackon state | Failed slur or rejected tackon attempts overwrite candidates. | [#154](https://github.com/mk270/whitakers-words/pull/154) |
| 5 | vis syncope | The rule accepts verbs outside the perfect system. | [#155](https://github.com/mk270/whitakers-words/pull/155) |
| 6 | Enclitic state | Failed attempts lose candidates; fixes-only lookup misses the base. | [Combined fixes][combined-fixes] |
| 7 | Unique part | The part is read from the uninitialized destination qualifier. | [Combined fixes][combined-fixes] |
| 8 | Compound numbers | Numeral-hit flags survive failed word splits and can cause a crash. | [Combined fixes][combined-fixes] |
| 9 | Repeated PACK/UNIQUES | The fixes-only pass repeats the ordinary unique and Qu lookups. | [Combined fixes][combined-fixes] |
| 10 | Repeated adverbs | Fix_Adverb derives multiple adverbs from the same adjective record. | [Combined fixes][combined-fixes] |
| 11 | Unique adverb ending | Fix_Adverb adds an ending absent from the unique's spelling. | [Combined fixes][combined-fixes] |
| 12 | Full enclitic result | Inserting a marker overruns a full 40-record trick array. | [Combined fixes][combined-fixes] |
| 13 | Packon boundary | A prefix match confuses the names cum and cumque. | [Combined fixes][combined-fixes] |
| 14 | Suffix fallback | A later suffix miss erases evidence of an earlier hit. | [Combined fixes][combined-fixes] |
| 15 | Qu grouping | Plain readings get sorted into the preceding add-on group. | [Combined fixes][combined-fixes] |
| 16 | Orphan add-ons | Trimming removes all readings but leaves their add-on markers. | [Combined fixes][combined-fixes] |
| 17 | Qu folding | The qu/cu test does not fold v to u. | [Combined fixes][combined-fixes] |
| 18 | Nested tackons | The outer tackon sweep drops an accepted inner tackon marker. | [Combined fixes][combined-fixes] |
| 19 | Ending lookup | The ending index uses the raw final letter, so final v finds none. | [Combined fixes][combined-fixes] |
| 20 | Lost full results (open) | Fixed parse arrays overflow; the exception is swallowed as UNKNOWN. | Open; excluded |

The first fix for #20 answered pilula and seconda but crashed on roboraria while grouping the output.
It is excluded from the comparison build and remains under investigation.

## Where we are now

These are the results of the final 0.3.0 report, `20261030T211216`.
The pinned collection contains 102 Latin books, with 4,662,505 word occurrences.

| Result | Tokens | Percentage |
| --- | ---: | ---: |
| Exact agreement | 4,642,934 | 99.58% |
| Differences | 19,571 | 0.42% |
| ↳ WORDS' guesses | 19,559 | |
| ↳ Other explained differences | 12 | |

Every word is counted, including WORDS' guesses, which whitakerLib doesn't currently implement.

Of the remaining 12 word occurrences, we've found the cause of each:

| Words | Occurrences | Cause |
| --- | ---: | --- |
| Seven run-together words over 24 letters (eosdemiuvencosetquattuoriugerum, ...) | 7 | The library's input limit; WORDS answers UNKNOWN. |
| pilula, seconda | 3 | A WORDS bug: the answer overflows WORDS' fixed parse arrays and is dropped silently, so WORDS says UNKNOWN. Our first fix gave regressions (WORDS crashed on another word in the books), so we are still looking at it. |
| basa, basin | 2 | WORDS prints a reading again for a second dictionary record that shares the stem; this grouping rule is not ported. |

The current accuracy only reflects the books tested so far. As more texts are added, the percentage may go down as we uncover new differences, not necessarily because the library has gotten worse.

If you'd like to add another text or book, feel free to open an issue. As long as it's publicly available and not behind a paywall, I'm happy to add it.

## Roadmap

A few things I'd like to add over time:
- **Runtime coverage:** Measure which library branches the books exercise,
  then add fixtures for uncovered paths or document why they are unreached.
- **More constructed words:** Test words built from WORDS' rules, not just words found in books.
- **More books:** Keep expanding the corpus to find new differences.
- **Lib against Lib:** Compare two versions of whitakerLib to catch changes between releases.

## Reports

The saved report above starts with:

```text
256045 spellings, 4662505 tokens

  maps: every field WORDS shows is equal    249262 spellings ( 97.35%)   4642934 tokens ( 99.58%)
  maps but for gaps                              0 spellings (  0.00%)         0 tokens (  0.00%)
  differs                                     6783 spellings (  2.65%)     19571 tokens (  0.42%)
    of which WORDS used a trick               6772 spellings (  2.64%)     19559 tokens (  0.42%)
```

Each run writes a new directory under `stored/reports/`:
- `summary.txt`: totals, field comparisons, unmatched lines and rule counts.
- `differences.tsv`: differing lines grouped, most tokens first.
- `differences_own.tsv`: differences without those explained by WORDS' guesses.
- `lines.txt`: differing spellings with WORDS and library lines side by side.
- `unmapped.tsv`: spellings that do not map, used for per-book counts.
- `rules.tsv` and `forms_rebuilt.txt`: comparison rules and dictionary-form checks.
- `oracle`: the tested library revision and capture path.
- `fixtures/`: the same reports for fixtures, outside book totals.

Use `oracle -r` for the latest summary and `oracle -s WORD` to inspect a
captured spelling, including its fields and pairings (`=` equal, `!` differs,
`~` gap). After a library change, compare the old and new report directories:
start with grouped differences, then inspect the affected spellings. A new
unmatched line or changed field gives a concrete place to investigate.
Pairing or shared-record inconsistencies invalidate the run.

## Files

- [oracle](oracle): registration, capture, build and comparison commands.
- [src/](src/): capture loading, Ada parsing, library layout, comparison and probe.
- [fixtures/](fixtures/): constructed inputs registered separately from books.
- `stored/books.tsv`: registered paths, hashes, book/fixture kind and probe selection.
- `stored/captures/<version>/`: token lists, provenance and shared raw answers
  under `answers/<batch>/`; older per-book answers remain readable.
- `stored/reports/`: comparison history; the latest full run also writes the
  [book list](../data/booklist.md).

Captures and reports are local generated artifacts, not bundled with the
source. Keep their saved answers and provenance intact. Texts come from the
Perseus Digital Library's canonical-latinLit collection (CC BY-SA 4.0);
the book list identifies the selection. The WORDS revision and fixes are
recorded above so results can be tied to the reference actually tested.

[combined-fixes]: https://github.com/ddd2951/whitakers-words/tree/combined_fixes
