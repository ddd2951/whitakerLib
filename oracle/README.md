# Lib Oracle

While working on whitakerLib I kept running into two issues:

- It's very easy to change one thing and end up affecting words you weren't expecting.
- The original Ada WORDS simply takes too long to rerun over a large collection of books after every small change.

So I made this oracle to try and fix both.

The oracle captures Ada WORDS' answers for the books you give it. It stores each word's answer once, even if the
word appears many times. I can then compare whitakerLib against those answers without rerunning Ada.

The comparisons use a [patched WORDS build][combined-fixes]. The [book list](../data/booklist.md) records the
texts and per-book results.

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

Register plain ASCII words, one per line.
`oracle -a -x FILE` registers a fixture outside the book totals. Missing, stale or incomplete captures stop a comparison.

## How it compares answers

WORDS prints its answers for an 80-column terminal and groups readings under dictionary entries. whitakerLib
returns records with typed fields and forms.

The oracle formats the library's answers using WORDS' printing rules, lines them up with the captured answers and
checks:
- Every line from WORDS has a corresponding line from the library, and vice versa.
- The readings appear in the same order.
- Every value WORDS shows corresponds to a library field with the same value.
- Readings sharing a dictionary entry agree on its fields.

The oracle also reads WORDS' dictionary records using the library's entry IDs. This lets it check dictionary fields
even when WORDS leaves out a repeated dictionary form line.

Things like spacing, how WORDS prints a value, or meanings cut short to fit the terminal don't count as
differences if the underlying values agree.

The report groups results as:
- **Match:** WORDS and the library agree.
- **Gap:** WORDS returns something the library doesn't support.
- **Difference:** Something else doesn't match.

As an example look at `arma`:
WORDS prints three noun readings under `armum`, then the imperative of `armo`.
The library returns two records containing four forms:

| WORDS reading | Library record | Form grammar |
| --- | --- | --- |
| arm.a N 2 2 NOM P N | 4769, armum | NOM P N |
| arm.a N 2 2 VOC P N | 4769, armum | VOC P N |
| arm.a N 2 2 ACC P N | 4769, armum | ACC P N |
| arm.a V 1 1 PRES ACTIVE IMP 2 S | 4770, armo | PRES ACTIVE IMP 2 S |

`oracle -s arma` shows all eight printed lines paired. These are four readings,
two dictionary forms and two meanings, with every field equal.

The layout follows WORDS' `Put_Parse_Details` and `Put_Dictionary_Form`,
independently of the captured answer. Lines are paired in order, then unmatched
lines of the same kind are compared field by field. Nothing left over is ignored.

The printing rules are deliberately narrow:
- Fold case and j/i, v/u where WORDS does.
- Strip a meaning of leading continuation bars and compare its first 79 characters.
- Allow an omitted person 0 and WORDS' blank deponent voice. Hidden values are not compared.
- Check constructed numeral meanings against the numeral's value and sort.

Each rule is counted in the report. Changes need review against the Ada routine.
The rule table is in [lines.hpp](src/lines.hpp), field comparisons in
[compare.cpp](src/compare.cpp), and layout in [library_lines.cpp](src/library_lines.cpp).

## Validating the oracle

The oracle checks that every field in `WhitakerRecord` and `WhitakerForm` is accounted for. The
[probe](src/probe.cpp) changes each mapped field, record and form to check that the comparison notices. Fields
WORDS doesn't show must leave the result unchanged.

Every run checks that both sides' lines are accounted for and that readings from the same dictionary entry agree.
It also checks the source data and rebuilds WORDS' numbered dictionary forms against the captures. When the oracle
changes, I compare reports to see which words get different results.

## Bugs found in WORDS

The comparisons have also uncovered bugs in WORDS. I've listed them below. I don't want to reproduce them in
whitakerLib just to get matching results.

The saved answers use my local WORDS build, `5dd8ca5`. Its source matches the `combined_fixes` branch of my fork,
[`ddd2951/whitakers-words`](https://github.com/ddd2951/whitakers-words/tree/combined_fixes), at `da6b35f`. The
captures keep the source revision and build details. The [WORDS fix notes](WORDS_FIXES.md) list the fixes and
regression examples.

The findings keep the numbers they were given when found. The build includes
fixes for all findings except #20, which is still being investigated.

| # | Finding | Cause | Upstream |
| --- | --- | --- | --- |
| 1 | Syncope state | Failed syncope attempts overwrite dictionary candidates. | [#151](https://github.com/mk270/whitakers-words/pull/151), merged |
| 2 | Enclitic syncope | Syncope is tried on the whole word instead of its base. | [#152](https://github.com/mk270/whitakers-words/pull/152), closed by mistake, not merged yet |
| 3 | Comment reader | A short line is tested using stale buffer text. | [#153](https://github.com/mk270/whitakers-words/pull/153), merged |
| 4 | Slur and tackon state | Failed slur or rejected tackon attempts overwrite candidates. | [#154](https://github.com/mk270/whitakers-words/pull/154), merged |
| 5 | vis syncope | The rule accepts verbs outside the perfect system. | [#155](https://github.com/mk270/whitakers-words/pull/155), merged |
| 6 | Enclitic state | Failed attempts lose candidates; fixes-only lookup misses the base. | [#157](https://github.com/mk270/whitakers-words/pull/157), merged |
| 7 | Unique part | The part is read from the uninitialized destination qualifier. | [#159](https://github.com/mk270/whitakers-words/pull/159), waiting for review |
| 8 | Compound numbers | Numeral-hit flags survive failed word splits and can cause a crash. | [#156](https://github.com/mk270/whitakers-words/pull/156), merged |
| 9 | Repeated PACK/UNIQUES | The fixes-only pass repeats the ordinary unique and Qu lookups. | Not posted yet |
| 10 | Repeated adverbs | Fix_Adverb derives multiple adverbs from the same adjective record. | Not posted yet |
| 11 | Unique adverb ending | Fix_Adverb adds an ending absent from the unique's spelling. | Not posted yet |
| 12 | Full enclitic result | Inserting a marker overruns a full 40-record trick array. | Not posted yet |
| 13 | Packon boundary | A prefix match confuses the names cum and cumque. | [#158](https://github.com/mk270/whitakers-words/pull/158), waiting for review |
| 14 | Suffix fallback | A later suffix miss erases evidence of an earlier hit. | Not posted yet |
| 15 | Qu grouping | Plain readings get sorted into the preceding add-on group. | Not posted yet |
| 16 | Orphan add-ons | Trimming removes all readings but leaves their add-on markers. | Not posted yet |
| 17 | Qu folding | The qu/cu test does not fold v to u. | Not posted yet |
| 18 | Nested tackons | The outer tackon sweep drops an accepted inner tackon marker. | Not posted yet |
| 19 | Ending lookup | The ending index uses the raw final letter, so final v finds none. | Not posted yet |
| 20 | Lost full results (open) | Fixed parse arrays overflow and the exception is swallowed as UNKNOWN. | Not posted yet |

The first fix for #20 answered pilula and seconda but crashed on roboraria while grouping the output.
It is excluded from the comparison build and remains under investigation.

## Where it is now

The latest comparison against patched WORDS `5dd8ca5` covers 102 pinned Perseus books, with 4,663,396 word occurrences.

| Result | Tokens | Percentage |
| --- | ---: | ---: |
| Exact agreement | 4,643,728 | 99.58% |
| Differences | 19,668 | 0.42% |
| ↳ WORDS' guesses | 19,665 | |
| ↳ WORDS parse-array overflow | 3 | |

The total includes WORDS' word-split and bad Roman numeral guesses. The other three differences are two occurrences
of *pilula* and one of *seconda*. WORDS fills its fixed parse arrays and drops the answer, giving `UNKNOWN`.
whitakerLib returns the readings for both spellings.

## Reports

Each run writes a new directory under `stored/reports/`:
- `summary.txt`: totals, field comparisons, unmatched lines and rule counts.
- `differences.tsv`: differing lines grouped, most tokens first.
- `differences_own.tsv`: differences without those explained by WORDS' guesses.
- `lines.txt`: differing spellings with WORDS and library lines side by side.
- `unmapped.tsv`: spellings that do not map, used for per-book counts.
- `rules.tsv` and `forms_rebuilt.txt`: comparison rules and dictionary-form checks.
- `oracle`: the tested library revision and capture path.
- `fixtures/`: the same reports for fixtures, outside book totals.

In the word view, `=` means equal, `!` means different and `~` means a gap. After a library change, compare the
old and new reports. Start with grouped differences, then inspect the affected words. Pairing or shared-record
inconsistencies invalidate the run.

Captures and reports are local generated artifacts, not bundled with the
source. Keep their saved answers and provenance intact. Texts come from the
Perseus Digital Library's canonical-latinLit collection (CC BY-SA 4.0).
The book list identifies the selection. The WORDS revision and fixes are
recorded above so results can be tied to the reference actually tested.

[combined-fixes]: https://github.com/ddd2951/whitakers-words/tree/combined_fixes
