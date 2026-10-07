# whitakerLib

[![CI](https://github.com/ddd2951/whitakerLib/actions/workflows/ci.yml/badge.svg)](https://github.com/ddd2951/whitakerLib/actions/workflows/ci.yml)

A small C library that takes a Latin word and returns every reading William
Whitaker's WORDS data supports: stem, meaning, part of speech and inflection,
add-ons and Roman numerals included.

- **Under a microsecond a word**, over eleven Latin books.
- **1,184,036 spellings, 2,378,514 readings** in a 10 MB image embedded in
  the library: no data file, no database, no initialization.
- **96.4% the same as the original Ada WORDS**; 99.84% leaving out WORDS'
  guesses.

## Why?

whitakerLib started as my own training project. I made it public because I
couldn't find anything that did exactly this: fast lookups, no dependencies,
little memory, and a plain C interface any language can call.

It isn't meant as an easy tool for translating Latin, but as the backend for
other things, such as running it on a Kindle or another low-resource device.

Most Latin tools won't need lookups this quick, and the generator could be
written without reflection. But both were fun to build. "Why reflection?" in
`codegen/README.md` tells that part.

## Arma virumque cano

The first three words of the *Aeneid*, as the library reads them:

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

Choosing one is the reader's part, as it always was.

## API

The project is pre-1.0: the API is small, but its ABI is not frozen yet.
Seven functions, in `include/whitaker.h`, which documents every field:

```c
WhitakerStatus whitaker_analyze(const char *word, WhitakerResult *out);
void whitaker_result_copy(WhitakerResult *dst, const WhitakerResult *src);
size_t whitaker_describe(const WhitakerGrammar *grammar, char *out,
                         size_t size);
const char *whitaker_name(WhitakerField field, uint8_t value);
const char *whitaker_addon_spelling(uint16_t id);
const char *whitaker_addon_meaning(uint16_t id);
WhitakerStatus whitaker_init(void); /* not needed; kept for 0.2 callers */
```

`whitaker_analyze()` returns `WHITAKER_OK` (with `count` 0 for an unknown
word), `WHITAKER_INVALID_ARGUMENT` for a null, `WHITAKER_INPUT_TOO_LONG` past
24 bytes. On an error `out` is untouched. `WHITAKER_MALFORMED_IMAGE` is never
returned: the image is checked when the library is built.

- **Strings are borrowed**, from the library or from the result's own `text`.
  Don't free them.
- **Copy a result with `whitaker_result_copy()`**, never `=` or `memcpy`: some
  strings point into the result itself.
- **A result is about 34 KiB**; keep it static or on the heap.
- **Threads:** safe, each thread with its own result. The C++ example runs
  eleven books on 12 threads with the same answers as one thread, and
  ThreadSanitizer finds no race.

## Example

```c
#include <stdio.h>

#include <whitaker.h>

int main(void) {
  static WhitakerResult result;

  if (whitaker_analyze("amo", &result) != WHITAKER_OK)
    return 1;

  for (int i = 0; i < result.count; ++i) {
    const WhitakerMatch *match = &result.matches[i];
    char line[64];
    whitaker_describe(&match->grammar, line, sizeof line);
    printf("%s | %s\n  %s\n", match->orth, line, match->meaning);
  }
}
```

```
am | V C1 V1 PRES ACTIVE IND 1 S
  love, like; fall in love with; be fond of; have a tendency to;
```

## What it does, and doesn't

- Every reading, in WORDS' order, with WORDS' add-ons: prefixes, suffixes,
  tackons, packons and tickons.
- Syncopated perfects read as their full forms, as in WORDS' main syncope
  pass: `amasti` as `amavisti`.
- Case-insensitive; `j` folds with `i` and `v` with `u`, except in Roman
  numerals and, as in WORDS, in the syncope pass. ASCII only.
- `orth` is the matched stem, not a dictionary headword.
- One word per call: no tokenizing, parsing, ranking or translation.
- Not a drop-in replacement for WORDS: same data, the same lookup rules
  followed closely, but not identical for every input.

## Speed and accuracy

Both were measured over eleven Perseus texts, 867,217 words: Caesar,
Cicero, Virgil, Ovid, Tacitus, Plautus, Horace, Phaedrus, Seneca, Suetonius
and all of Livy.

**Accuracy.** Against the Ada WORDS program, built from the same data
revision:

- **96.4%** of the words get the same readings (part of speech and
  inflection; stems, meanings and order not compared), from 95.5% for Horace
  to 99.3% for Phaedrus.
- **2.6%**, mostly names, neither knows.
- Of the rest, 83% are WORDS guessing at a word it doesn't know, by splitting
  it in two or changing letters, which the library doesn't do. Leaving those
  out, the two agree on **99.84%**.

**Speed.** The 867,217 words and their 4,229,084 readings take 594 ms one by
one, as the C example runs them, and about 92 ms on the CPU's 12 threads, as
the C++ example runs them with `std::async`, describing and checking every
reading as well. GCC 16.2.1 Release on an AMD Ryzen 5 5600X (6 cores), the
performance governor, one-thread runs pinned to one core, medians of five
runs: one machine's measurement, not a guarantee.
The C example reports its time, on stderr, for any text:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build && ctest --test-dir build -R examples
tr -cs 'A-Za-z' '\n' < book.txt |
  build/examples/build/whitaker-c-example > /dev/null
```

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

## Data and licence

The data is from Colonel William Whitaker's WORDS, revision `1f2f0fb0867a`
(2026-08-26) of
[`mk270/whitakers-words`](https://github.com/mk270/whitakers-words):
`DICTLINE.GEN`, `INFLECTS.LAT`, `UNIQUES.LAT` and `ADDONS.LAT`, vendored
under `data/source/` with `MANIFEST.sha256`.

`data/whitaker.dat`, the generated image: 10,472,992 bytes, SHA-256
`7e9ab3fbd8060b6a90946b819aceafe8c3adcb943ade0613f55f2c37020a7f6b`.
`codegen/README.md` says how it is made and checked; it is checked in, so a
build cannot change it silently.

The library is under the European Union Public Licence v. 1.2 (`LICENSE`).
The WORDS data is a separate work under Whitaker's notice
(`data/source/LICENCE.txt`).
