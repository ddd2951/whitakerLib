# whitakerLib

[![CI](https://github.com/ddd2951/whitakerLib/actions/workflows/ci.yml/badge.svg)](https://github.com/ddd2951/whitakerLib/actions/workflows/ci.yml)

A small C library that takes a Latin word and returns every reading William
Whitaker's WORDS data supports: stem, meaning, part of speech and inflection,
add-ons and Roman numerals included.

- **Under a microsecond a word**, on average over [102 Latin books](data/booklist.md#books).
- **1,205,496 spellings, 2,374,470 readings** in a 10 MB image embedded in
  the library: no data file, no database, no initialization.
- **99.58% of words answered exactly as the Ada WORDS answers them**: every
  reading, record and add-on, in WORDS' order. The [oracle](oracle/README.md)
  explains the comparison.

## Why?

whitakerLib started as my own training project. I made it public because I
couldn't find anything that worked like this: fast lookups, no dependencies,
low memory usage, and a plain C interface any language can call.

It isn't meant as an easy tool for translating Latin, but as the backend for
other things, such as running it on a Kindle or another low-resource device.

Most Latin tools won't need lookups this quick and the generator could have been
written without reflection, but both were fun to build. The reasons why are
given in "Why reflection?" in `codegen/README.md`.

## Arma virumque cano

The first three words of the *Aeneid*. This is how the library reads them:

- **arma**: "arms, weapons", as nominative, vocative or accusative plural,
  or the imperative of *armo*: "arm!"
- **virumque**: the tackon *-que*, "and", comes off and leaves *virum*, read
  as *vir*, "man, hero", alongside *virus* and the old genitive plural of
  *vis*, "strength".
- **cano**: "I sing", but also the dative or ablative of *canus*, "gray hair,
  old age", of *canum*, "wicker basket", and of the adjective *canus*,
  "white, hoary".

So as far as single words go, Virgil might be singing of arms and the man, or
telling you to arm the man for the wicker basket. Or for old age. Or for the
white one: *cano* won't say which. The library gives you every reading.

Choosing the meaning is the reader's part, as it always was.

## API

The project is pre-1.0: the API is small, but its ABI is not frozen yet.

The API consists of eleven functions, found in `include/whitaker.h`, which documents every field:

```c
WhitakerResult *whitaker_result_new(void);
void whitaker_result_free(WhitakerResult *result);
void whitaker_result_clear(WhitakerResult *result);
WhitakerResult *whitaker_result_clone(const WhitakerResult *result);

// whitaker_analyze() replaces what the result holds with one word.
WhitakerStatus whitaker_analyze(const char *word, WhitakerResult *result);

// whitaker_analyze_append() adds one word after the words already there.
// Both return WHITAKER_OK (an unknown word is a word with no records),
// WHITAKER_INVALID_ARGUMENT for a null, WHITAKER_INPUT_TOO_LONG past 24
// bytes, or WHITAKER_OUT_OF_MEMORY. On an argument error the result is untouched.
WhitakerStatus whitaker_analyze_append(const char *word, WhitakerResult *result);
size_t whitaker_describe(const WhitakerGrammar *grammar, char *out,
                         size_t size);
const char *whitaker_name(WhitakerField field, uint8_t value);
const char *whitaker_addon_spelling(uint16_t id);
const char *whitaker_addon_meaning(uint16_t id);
WhitakerSpellingRule whitaker_spelling_rule(uint8_t rule);
```

A result holds words, each word holds its records, each record holds its
forms, using the same format as WORDS. For example, `arma` is one word with two records,
`armum` with three forms (nominative, vocative, accusative plural) and `armo`
with one.

- **A word** (`words[w]`) says where its records start, how many there are,
  and whether rarer readings were left out (`trimmed`, WORDS' "*").
- **A record** (`records[r]`) is a dictionary entry as the word reaches it. It contains
  the entry (`entry`, whose `id` names the record as WORDS numbers it), its
  meaning and part, the add-ons around the stem, and how it was found
  (`route`): `DIRECT`, one of the `SYNCOPE_*` rules, `ADVERB`, or a spelling
  change (`SLUR` or `WORD_MOD`). `spelling_rule` identifies the change;
  `whitaker_spelling_rule()` returns its strings, direction and kind. The record also says
  where its forms start and how many there are.
- **A form** (`forms[f]`) is one line under it: the stem and ending as looked
  up (`orth`, `ending`), the grammar, and the inflection's age and frequency.

- **A result grows** to contain what the words need and keeps that memory
  through `whitaker_result_clear()`; counts are limited by available memory.
- **Read `words`, `records` and `forms` again after each call**: they may
  move when the result grows; the indexes in them stay valid. Strings stay
  put until the result is cleared or freed.
- **Strings and add-ons are borrowed**, from the library or from the result.
  Don't free them. To keep an independent result, clone it. Cloning can return `NULL` on allocation failure.
- **Threads:** safe, each thread with its own result. The C++ example compares
  threaded and sequential answers for the texts supplied.

## Example

```c
#include <stdio.h>

#include <whitaker.h>

int main(void) {
  WhitakerResult *result = whitaker_result_new();

  if (result == NULL)
    return 1;
  if (whitaker_analyze("amoque", result) != WHITAKER_OK) {
    whitaker_result_free(result);
    return 1;
  }

  for (size_t r = 0; r < result->record_count; ++r) {
    const WhitakerRecord *record = &result->records[r];
    printf("record %u: %s\n", record->entry.id, record->meaning);
    for (size_t f = 0; f < record->form_count; ++f) {
      const WhitakerForm *form = &result->forms[record->first_form + f];
      printf("  %s + %s\n", form->orth, form->ending);
    }
  }
  whitaker_result_free(result);
}
```

The [oracle](oracle/README.md) compares the library's fields with saved WORDS
answers. `oracle/oracle -s amoque` shows the lines and their pairings.

## What it does, and doesn't

- Returns every reading, in WORDS' order, with WORDS' add-ons: prefixes, suffixes,
  tackons, packons and tickons.
- Syncopated perfects read as their full forms, as in WORDS' main syncope
  pass: `amasti` as `amavisti`. `route` says which rule is used.
- Case-insensitive. `j` folds with `i` and `v` with `u`, except in Roman
  numerals and, as in WORDS, in the syncope pass. ASCII only.
- `orth` is the matched stem and `ending` the inflection after it (`am` + `o`),
  not a dictionary headword.
- One word per call: no tokenizing, parsing, ranking or translation.
- WORDS' word-split and bad Roman numeral guesses are not implemented;
  see the [roadmap](#roadmap).
- Not a drop-in replacement for WORDS: same data, the same lookup rules
  followed closely, but not identical for every input.

## Speed and accuracy

Both were measured over the same [102 Latin books](data/booklist.md#books),
totaling 4,662,505 word occurrences.

**Speed.** On one thread, the C example analyzes the 4,662,505 words and
returns 17,460,251 readings in 4,236.4 ms, about 0.91 µs a word. This times
`whitaker_analyze`, excluding input and printing. On the CPU's 12 threads,
the C++ example runs them with `std::async` in 624.3 ms, including describing
and fingerprinting every reading. It then checks the answers against one
thread.

Timing the whole C pipeline from outside, including input and formatting
output to `/dev/null`, took 6,685.4 ms, about 1.43 µs a word, in one
additional run.

That is GCC 16.2.1 Release on an AMD Ryzen 5 5600X (6 cores), with the
performance governor and the C example pinned to one core. The C analysis
and C++ threaded figures are medians of five runs: one machine's measurement,
not a guarantee. The C example prints its time on stderr, so you can time
any text:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build && ctest --test-dir build -R examples
tr -cs 'A-Za-z' '\n' < book.txt |
  build/examples/build/whitaker-c-example > /dev/null
```

**Accuracy.** Every word is compared with the answer from the Ada WORDS
program, built using the same data: each reading with its stem, each
dictionary record and meaning, the add-ons, and the order of all of them.

**99.58%** of the words (97.35% of the distinct spellings) get exactly WORDS'
answer. The [book list](data/booklist.md#books) gives the per-book results;
the [oracle README](oracle/README.md#where-we-are-now) explains the remaining
differences. That comparison uses the patched WORDS revision `5dd8ca5`,
described in the [oracle README](oracle/README.md#bugs-found-in-words).

Five fixes have been sent upstream
([#151](https://github.com/mk270/whitakers-words/pull/151),
[#152](https://github.com/mk270/whitakers-words/pull/152),
[#153](https://github.com/mk270/whitakers-words/pull/153),
[#154](https://github.com/mk270/whitakers-words/pull/154),
[#155](https://github.com/mk270/whitakers-words/pull/155)).

## Inside

The steps a word goes through, in WORDS' order:

```text
whitaker_analyze("adiitque")
  findReadings
    addWordReadings           the dictionary, qu- pronouns, packons, tackons
    addRomanNumeral
    addPrefixVariantReadings  only if nothing yet: inp- as imp-, adt- as att-
    addSyncopeReadings        amasti as amavisti
    addEncliticReadings       -que, -ne, -ve, -est off: adiit + que
    addWordReadings           only if nothing yet: with prefixes and suffixes
    addEncliticReadings       if still nothing: with prefixes and suffixes
  addRespelledReadings        if still unknown and not a name: ae as e, ph as f, ...
  addRespelledEncliticReadings if respelling found nothing: try without an enclitic
  addAdverbsOfAdjectives      fidissimus gives fidissime
```

The dictionary lookup itself is done ahead of time: `codegen` runs it over
every stem and ending and stores the answers in the image
(`codegen/README.md`).

For readers of the Ada source:

| WORDS procedures | Library source |
| --- | --- |
| Parse_Latin_Word, Is_Capitalized | [whitaker.cpp](src/whitaker.cpp) |
| Pass, Enclitic, Roman_Numerals, Perform_Syncope | [find_readings.cpp](src/parse/find_readings.cpp) |
| Word, Process_Packons, Try_Tackons | [word_readings.cpp](src/parse/word_readings.cpp) |
| Try_Slury, Try_Tricks, Tricks_Enclitic | [respellings.cpp](src/parse/respellings.cpp) |
| Apply_Prefix, Apply_Suffix | [addon_readings.cpp](src/search/addon_readings.cpp) |
| List_Sweep, Order_Parse_Array | [facts.hpp](src/facts.hpp), [latin.hpp](latin/latin.hpp) |
| Handle_Adverb, Fix_Adverb | [adverb.cpp](src/parse/adverb.cpp) |

## Roadmap

- **0.3.1:** Python, WebAssembly and Lua bindings.
- **0.3.2:** macrons, and WORDS' guesses (word splits and bad Roman numerals).
- **0.3.3:** more books, and more of their words sorted out.

## Build

Tested on Linux only. It needs CMake 3.21, GCC 15 or Clang 20 (for `#embed`
in C++, with its offset parameter), and `nm` for the tests. Callers only need
C.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --install build --prefix /your/prefix
```

Options: `WHITAKER_SANITIZE` (ASan and UBSan), and `WHITAKER_BUILD_TESTING`,
`WHITAKER_INSTALL` and `WHITAKER_WERROR`, which are on when whitakerLib is
the top-level project and off under `add_subdirectory()`.

The install provides a CMake package and a relocatable `whitaker.pc`:

```cmake
find_package(Whitaker CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE Whitaker::whitaker)
```

`examples/` has a C program that prints every reading of its words, one by
one, and a C++ one that analyzes books on all the CPU's threads with
`std::async` and checks the answers against one thread. `ctest` builds both
against an install and compares their output with `examples/expected/`.

## Contributing

It isn't an easy project to jump into yet, but I want it to become one people
can join. From 0.3.0 on, the work goes in as many small commits instead of a
few big ones, which should make it easier to follow.

**Words.** If the library reads a word from a book differently than WORDS
does, open an issue or a pull request. A link to the book, or a picture of the
page with the word on it, is enough.

**Code.** Anything that makes it faster or smaller is welcome, as long as I
can understand it. I'm not strict on style either; if you know a cool trick,
show me. How you got there doesn't matter, AI included, as long as you
understand the change you're sending.

## Data and licence

The data is from Colonel William Whitaker's WORDS, revision `1f2f0fb0867a`
(2026-08-26) of
[`mk270/whitakers-words`](https://github.com/mk270/whitakers-words):
`DICTLINE.GEN`, `INFLECTS.LAT`, `UNIQUES.LAT` and `ADDONS.LAT`, vendored
under `data/source/` with `MANIFEST.sha256`.

`data/whitaker.dat`, image format 10: 10,527,058 bytes, SHA-256
`35fbe999b0100f62c2d3630bd62d93ca31670c7a76e220e3c2b48dc4d88a4630`.
`codegen/README.md` says how it is made and checked; it is checked in, so a
build cannot change it silently.

The library is under the European Union Public Licence v. 1.2 (`LICENSE`).
The WORDS data is a separate work under Whitaker's notice
(`data/source/LICENCE.txt`).
