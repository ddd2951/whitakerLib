# whitakerLib lookup-image generator

`gen` builds `data/whitaker.dat`, the lookup image the library embeds.
It takes no options: what it builds comes from the vendored sources and
their schemes, so change those instead. It works and its output is checked
exhaustively, but it is far from finished.

## Inputs

`gen` runs from the repository root and expects the files as follows:

```text
data/source/DICTLINE.GEN   39,335 lines
data/source/INFLECTS.LAT    3,228 lines, 1,797 records
data/source/UNIQUES.LAT       237 lines,    79 records
data/source/ADDONS.LAT      1,200 lines,   343 records
```

The line and record counts are constexpr, so adding or removing a word means
changing them. A new understanding or change of what a word means does not.

## Output

`gen` will write `data/whitaker.dat`. The write only happens after it has
verified it, so a failed run does not replace the old one (see Checks).

## Build and run

Needs a POSIX system, CMake 3.28 or newer and GCC 16 with `-freflection`
(C++26 reflection). Tested on Linux only, would love to hear if anyone gets it
to build on macOS or a BSD. From the repository root:

```sh
cmake -S codegen -B build-codegen
cmake --build build-codegen --target image
```

The build is Release unless another type is given. The `image` target builds
`gen` and runs it from the repository root.

`sha256sum data/whitaker.dat` with the pinned corpus should give:

```text
size:    10,472,992 bytes
SHA-256: 7e9ab3fbd8060b6a90946b819aceafe8c3adcb943ade0613f55f2c37020a7f6b
```

A warm Release run takes a median 3.4 seconds on an AMD Ryzen 5 5600X with
GCC 16.2.1, pinned to one core.

## Pipeline

`main()` runs four phases in order:

1. `source::init()` reads the four files into lines and frames their records.
2. `word::init()` parses every record once into typed values.
3. `expand::init()` applies the WORDS morphology rules and decides which forms
   and readings are kept.
4. `emitter::emit()` builds the automaton and tables, writes the image and
   checks it.

Each phase keeps what it builds until `gen` exits and answers through plain
functions on typed indices: `source::dictline(index)`, `word::entry(row)`,
`expand::spelling(result)`. They hand out views, not copies: a word's text is a
`string_view` into the source lines, a spelling a view into expansion's store.
Nothing is copied between phases and nothing is freed early.

The price: a phase answers only after its `init()` has run. Before that its
storage is there but empty, and a call returns empty text or a default record,
or reads out of bounds, without complaint. Nothing checks the order; `main()`
is the only place that states it.

## Why reflection?

Reading Whitaker's WORDS from C++ is a problem I keep coming back to, as a place
to try out new things I've learned. With reflection such an exciting addition
to C++, it was natural for this project to start asking what it could do here.
The first use was replacing magic_enum for turning an `enum class` into a
`string_view`. The first public version, 0.1, was already built this way.
Since then, a few places have shown where it really shines:

- **Columns on the struct.** DICTLINE's fixed columns are annotations on the
  record's members (`[[= Column{0, 19}]] Stem stem1;`); the parser reads them
  from there.
- **Records filled member by member.** A `template for` over a struct's members
  parses each field into the next member; a new member needs no parser change.
- **Enum names are the source tokens.** `N`, `ADJ`, `TRANS` are read and
  printed straight from the enumerators; no name tables.
- **Types know their part of speech.** `[[= Part::N]] struct Noun`, read by
  `latin::partOf`, in place of hand-written switches.
- **One index type per source file.** `source::Index<^^scheme::dictline>`: a
  DICTLINE index can't be passed where an INFLECTS one is expected.
- **The C header checked against gen.** `whitaker.h`'s C enums mirror gen's,
  names and values, in one loop instead of 197 lines of asserts.
- **Any value described.** Error reports print a record field by field, and
  name exactly the fields that differ.

The cost: GCC 16 only, and `-Wno-shadow`, since every `template for` trips
`-Wshadow`. Editors lag behind: clangd only gets close with the
[clang-p2996](https://github.com/bloomberg/clang-p2996) fork, and still trips
over annotations through a PCH bug I'm working on a fix for. The library itself
is C++23 and has no reflection; only `gen` uses it.

## Checks

- **Build:** the C enums in `whitaker.h` mirror gen's
  (`src/types/latin_test.cpp`).
- **Load:** the image loads as the library would (header, directory, section
  sizes).
- **Spellings:** every kept spelling, in order, by walking the whole automaton.
- **Rows:** every row of each spelling, in order: its stem, meaning and grammar.
- **Entries:** all 39,415 stored entries match the parsed DICTLINE and UNIQUES
  words.
- **Fallback:** the records only the fallback reads stay inside their tables,
  and the endings and fallback stems are sorted. Which rows they hold is
  pinned by the library's examples test (`examples/expected/c.txt`), not here.

All but the first run on a temporary file beside the image
(`data/.whitaker.dat.tmp.<n>`). Only an image that passes every check replaces
`data/whitaker.dat`. The first failure stops `gen`; the old image is untouched
and the failed one stays for inspection.

## Updating the corpus

To try a newer WORDS:

1. Update the files under `data/source/` and their hashes in
   `data/source/MANIFEST.sha256`; note the upstream revision in the root
   README under "Data and licence".
2. Update the counts in the schemes under `src/source/`, and in Inputs above.
3. Build the `image` target, which replaces `data/whitaker.dat` only when
   every check passes; then build the library and run its tests.

The checks prove the image holds exactly what expansion built. They do not
prove a corpus change is linguistically right, nor that every WORDS front-end
behaviour is reproduced.
