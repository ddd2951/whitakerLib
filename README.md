# whitakerLib

[![CI](https://github.com/ddd2951/whitakerLib/actions/workflows/ci.yml/badge.svg)](https://github.com/ddd2951/whitakerLib/actions/workflows/ci.yml)

whitakerLib is a small C library for one job: take a Latin word and return all
morphological readings supported by a pinned copy of William Whitaker's WORDS
data. Each reading includes the matched stem, English meaning, part of speech,
and inflection. WORDS add-ons and Roman numerals are also supported.

The library reports what it finds without using sentence context to choose a
preferred reading. That narrow scope is intentional.

The project is pre-1.0. The current API is small, but its ABI and analysis
contract are not frozen yet.

## A small example

```c
#include <stdio.h>

#include <whitaker.h>

int main(void) {
  static WhitakerResult result;

  if (whitaker_init() != WHITAKER_OK)
    return 1;
  if (whitaker_analyze("amo", &result) != WHITAKER_OK)
    return 1;

  for (int i = 0; i < result.count; ++i) {
    const WhitakerMatch *match = &result.matches[i];
    char line[64];
    whitaker_describe(&match->grammar, line, sizeof line);
    printf("%s | %s | %s\n", match->orth, match->pos, line);
    printf("  %s\n", match->meaning);
  }
}
```

For `amo`, one result is the stem `am`, part of speech `V`, and inflection
`V C1 V1 PRES ACTIVE IND 1 S`, with its English meaning.

## What it does

- Analyzes one NUL-terminated Latin word per call and returns every retained
  reading in source-derived order.
- Returns the matched stem, English meaning, part of speech, and compact WORDS
  morphology for each reading.
- Handles WORDS add-ons: prefixes, suffixes, tackons, packons, and tickons.
- Treats ASCII case as insignificant and folds `j` with `i` and `v` with `u`.
- Recognizes the Roman-numeral forms accepted by WORDS.
- Embeds all lookup data in the shared library. Normal use opens no data file
  and needs no database or network service.

The current image contains 1,184,036 spellings and 2,378,514 ordered analysis
rows. Its size is 11,133,203 bytes.

## What it does not do

- It does not tokenize, parse, rank readings from context, or translate prose.
- It does not search English definitions.
- It does not return a normalized dictionary headword. `orth` is the matched
  stem and can be shorter than the query.
- It does not accept arbitrary Unicode Latin text. The lookup alphabet is the
  ASCII alphabet used by the source corpus.
- It is not a drop-in replacement for the original WORDS program. The project
  uses the same data and follows its lookup rules closely, but it does not
  claim identical behavior for every possible input.
- It does not update its corpus at run time. A new corpus requires rebuilding
  the checked-in image with the separate generator.

WORDS can contain several readings for one spelling, including readings with
the same morphology but different records or meanings. The library preserves
them instead of guessing which one the caller wants.

## Measured lookup speed

- **Workload:** 51,300 words from Caesar's *De bello Gallico*, producing
  264,791 readings whose fields were all consumed.
- **Warmed lookup:** 214 ms median, or 4.2 microseconds per word and about
  240,000 words per second.
- **Initialization and validation:** about 49 ms.
- **Environment:** seven-run GCC 16.2.1 Release benchmark on an AMD Ryzen 5
  5600X. The CPU was not pinned; its governor was `powersave`, with boost on.

These are single-machine measurements, not portable guarantees.

## Requirements

Building the library currently requires:

- GNU/Linux;
- CMake 3.20 or newer;
- GCC or Clang with C++23 support;
- a GNU-compatible assembler with `.incbin` support; and
- `nm` (binutils or LLVM) to run the tests.

The implementation uses C++23, but the installed API is C; callers do not need
to use C++23.

## Build and test

From this directory:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

The checked-in `data/whitaker.dat` is sufficient for this build. The generator
is not built or run as part of the library build.

Useful options:

- `-DWHITAKER_SANITIZE=ON` enables AddressSanitizer and
  UndefinedBehaviorSanitizer.
- `-DWHITAKER_BUILD_TESTING=OFF` omits the tests.
- `-DWHITAKER_INSTALL=OFF` omits install and package rules.
- `-DWHITAKER_WERROR=OFF` keeps compiler warnings non-fatal.

Tests, install rules, and fatal warnings default to on when whitakerLib is the
top-level CMake project. They default to off when another project adds this
directory with `add_subdirectory()`.

## Install and use from CMake

```sh
cmake --install build --prefix /your/prefix
```

An installed CMake consumer can use:

```cmake
find_package(Whitaker CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE Whitaker::whitaker)
```

The install also provides a relocatable `whitaker.pc` file:

```sh
cc example.c $(pkg-config --cflags --libs whitaker)
```

Complete C and C++ consumers are in `examples/`. After installation:

```sh
cmake -S examples -B build-examples \
  -DCMAKE_PREFIX_PATH=/your/prefix
cmake --build build-examples
```

## API contract

The public API is declared in `include/whitaker.h` and consists of seven
functions:

```c
WhitakerStatus whitaker_init(void);
WhitakerStatus whitaker_analyze(const char *word, WhitakerResult *out);
void whitaker_result_copy(WhitakerResult *dst, const WhitakerResult *src);
const char *whitaker_name(WhitakerField field, uint8_t value);
size_t whitaker_describe(const WhitakerGrammar *grammar, char *out,
                         size_t size);
const char *whitaker_addon_spelling(uint16_t id);
const char *whitaker_addon_meaning(uint16_t id);
```

Call `whitaker_init()` before the first analysis. It validates the embedded
image once. Repeated calls are harmless, including calls from different
threads.

`whitaker_analyze()` has these outcomes:

- `WHITAKER_OK`: the call completed. `out->count` can be zero when no reading
  was found.
- `WHITAKER_NOT_INITIALIZED`: `whitaker_init()` has not completed.
- `WHITAKER_INVALID_ARGUMENT`: `word` or `out` is null.
- `WHITAKER_INPUT_TOO_LONG`: the input is longer than 24 bytes.
- `WHITAKER_MALFORMED_IMAGE`: validation rejected the embedded data.

On an error, `out` is not changed.

Each match contains:

- `orth`: the matched stem, not necessarily the queried spelling or a lemma;
- `meaning`: the English meaning stored with the dictionary record;
- `pos`: the WORDS part-of-speech abbreviation;
- `grammar`: the morphological description as values, one byte a field,
  each holding the `Whitaker*` enum it names (`part`, `which` for the
  declension or conjugation, `variant`, `case_of`, `number`, `gender`,
  `comparison`, `numeral_sort`, `tense`, `voice`, `mood`, `person`); a field
  the reading has no use for is 0;
- `entry`: the dictionary entry's part, declension or conjugation (`which`),
  variant, gender, kind, comparison, numeral sort and value, area, geography,
  source, age and frequency. `kind` is interpreted by `part`: noun, pronoun,
  packon or verb kind. Unused fields are 0. Roman numerals have part NUM,
  `which` 2, sort CARD, their numeric value, and X labels; and
- `addons`: the derivation steps used for a prefixed, suffixed, or compounded
  reading, each an addon `id` and its `WhitakerAddonKind`.

`whitaker_name()` gives the name WORDS prints for a grammar or entry enum
value (`"NOM"` for `WHITAKER_CASE_NOM`), or null for a value the field doesn't
have. `whitaker_describe()` writes the whole line WORDS prints for a
`grammar`, the way `snprintf` does: at most `size` bytes including the
terminator, returning the length of the whole line.

`whitaker_addon_spelling()` and `whitaker_addon_meaning()` give a step's text
from its `id`, or null for an id the library doesn't have. An id is only
meaningful to the library that returned it; don't store it.

The strings are borrowed. They point either into the library's embedded image
or into the `text` storage of the `WhitakerResult` that owns them. Do not free
the strings.

Do not copy a populated `WhitakerResult` with assignment or `memcpy`. Some of
its pointers can refer to its own `text` array and will still refer to the
original object after a shallow copy. Use `whitaker_result_copy()` instead; it
copies the result and rebases those pointers into the destination's `text`
array. Passing a null pointer or the same object as both arguments is harmless.

A result holds at most `WHITAKER_MAX_MATCHES` (256) readings. A word with more
is cut at that many; the largest count in the current corpus is 88. A reading
holds up to `WHITAKER_MAX_ADDON_STEPS` (12) addon steps, enough for any word:
each tickon, tackon or packon removes at least two letters. A result is about
34 KiB on a 64-bit build, so static or heap storage is often more suitable
than a small thread stack.

`whitaker_analyze()` is safe to call from several threads at once, each with
its own `WhitakerResult`. The library holds no state but the validated image.

## Accuracy and evidence

The image is checked in so builds cannot silently change the corpus. During
generation, it is read back through the runtime reader and compared with the
generator's spelling, row, and rendered-field inputs. Runtime tests cover
lookup, add-ons, Roman numerals, spelling folding, errors, malformed images,
and the no-file-I/O boundary.

Version 0.1 of the library was also compared with the original Ada program
over the 51,300 words of Caesar's *De bello Gallico*. For 49,885 words
(97.2%), both returned the same non-empty set of part-of-speech and inflection
descriptions after documented display normalizations; both left another 373
unanswered. Stems, meanings, duplicate counts, and result order were not
compared.

Most remaining differences come from three WORDS fallback rules that the
library does not implement: syncope, two-word guesses, and internal letter
substitutions. Another group consists of one-letter inputs that the Ada front
end reserves for commands but the library treats as Latin. The comparison and
every difference are described in `test/corpus/README.md`.

This supports the behavior claimed here, not complete compatibility with the
WORDS front end.

## Data provenance

The source corpus comes from Colonel William Whitaker's WORDS, using revision
`1f2f0fb0867a` (2026-08-26) of
[`mk270/whitakers-words`](https://github.com/mk270/whitakers-words).

The four source inputs and their licence are vendored under `data/source/`
with `MANIFEST.sha256` beside them; `sha256sum -c MANIFEST.sha256` checks
them.

| File | Contents | Bytes |
| --- | --- | ---: |
| `DICTLINE.GEN` | 39,335 dictionary records | 6,115,855 |
| `INFLECTS.LAT` | 1,797 inflection rows | 129,314 |
| `UNIQUES.LAT` | 79 unique analyses | 9,642 |
| `ADDONS.LAT` | prefixes, suffixes, tackons, and packons | 34,697 |
| `LICENCE.txt` | Whitaker's notice | 696 |

`data/whitaker.dat` is the generated lookup image: 11,133,203 bytes, SHA-256
`7376d2f43b2f831aecc9d268af33b92f08fcf4684a2389314d95b33375c5c6da`. Its
workflow is documented in `codegen/README.md`.

## Licence

The library code is licensed under the European Union Public Licence v. 1.2.
See `LICENSE` for the complete terms.

The WORDS data is a separate work distributed under William Whitaker's notice.
See `data/source/LICENCE.txt`.
