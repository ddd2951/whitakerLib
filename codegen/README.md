# whitakerLib lookup-image generator

Gen builds `data/whitaker.dat`, the lookup image the library embeds, from
WORDS' four source files, fetched from upstream (see Getting WORDS' files). It
uses three names, one for each part.

- `gen_image` is the generator, one executable. It expands the sources, builds
  the image, checks it and publishes it.
- `image` is the CMake target that builds `gen_image` and runs it. Only an
  image that passes every check replaces `data/whitaker.dat`.
- `gen` is the CMake target that only builds `gen_image` and checks that the
  checked-in files are current. Its build refuses stale bindings and reading
  shapes, and it checks the two bounds and the image against the sources. It
  writes nothing.

The two-ended tables are image sections, so there is no checked-in C++ table and
no runtime table construction.

## Inputs

`gen_image` and the bounds checks run from the repository root and expect these
files.

```text
data/source/DICTLINE.GEN   39,335 lines
data/source/INFLECTS.LAT    3,228 lines, 1,797 records
data/source/UNIQUES.LAT       237 lines,    79 records
data/source/ADDONS.LAT      1,200 lines,   343 records
```

The line and record counts are constexpr, so adding or removing a word means
changing them. A new understanding or change of what a word means does not.

## Output

`image` writes `data/whitaker.dat`. The write only happens after the image has
been verified, so a failed run does not replace the old one (see Checks).

## Build and run

Needs a POSIX system, CMake 3.28 or newer and GCC 16 with `-freflection`
(C++26 reflection). Tested on Linux only, would love to hear if anyone gets it
to build on macOS or a BSD. From the repository root, run

```sh
cmake -S codegen -B build-codegen
cmake --build build-codegen --target gen     # check that everything is current
cmake --build build-codegen --target image   # rebuild data/whitaker.dat
```

The build is Release unless another type is given. `image` alone is enough when
only the image is stale. After a change to the header, the Latin declarations or
the sources, regenerate everything gen writes, in the order below. `gen_image`
does not build while the first two are stale, and it builds the image with the
two bounds compiled in (`gen` reports them stale).

```sh
cmake --build build-codegen --target write_reading_format    # include/latin_generated.hpp
cmake --build build-codegen --target write_bindings          # the C and Python bindings, the inspector's API view
cmake --build build-codegen --target write_two_ended         # the two-ended tables' bounds
cmake --build build-codegen --target write_reading_capacity  # the change list's bounds
cmake --build build-codegen --target image                   # data/whitaker.dat
```

## Pipeline

Gen is 68 steps, each a function declared in one file,
`src/execution_steps/steps.hpp`, in 12 phases. They read and parse WORDS' four
files, expand every stem and ending into WORDS' readings, then build, encode,
check and publish the image. `main()` is one line.

```cpp
runner::runParallel<^^execution_steps>();
```

Each step is tagged with the steps it follows and the values it reads and
writes.

```cpp
[[ = tag::Gate{^^checkImageReadings}, = tag::Gate{^^checkDictionaryWords},
   = tag::Gate{^^checkDictionaryWordStems}, = tag::Gate{^^checkAddonTables},
   = tag::Reads{^^storage::destination},
   = tag::Reads{^^storage::temporaryImage} ]] void publishImage();
```

At build time `src/execution_steps/run.hpp` collects the steps by reflection,
checks that every gate names a step and that the waits have no cycle, direct
or through other steps (the build stops if they do), and works out the order
and what may run at once. A step starts as soon as everything it waits for has
finished. Gen produces the same image byte for byte, run after run. Nothing
else lists the steps.

The weak point is that the Reads and Writes tags are kept by hand on each
declaration in `steps.hpp`, written from reading each step's code. The build
checks that each one names a real value, not that the list is complete, and a
missing one can let two steps race. A ThreadSanitizer build of gen helps find
it. Generating the tags from what each step actually touches is planned.

The values the steps share live in `src/storage/`, one header each, and stay
until gen exits. Steps hand out views, not copies. A word's text is a
`string_view` into the source lines, and a spelling a view into expansion's
store.

Every answer is derived twice, on purpose. The lookup tables are built from the
unswept licensing, and the lookup sweeps at run time. Expansion's finished
answers, in WORDS' own order, are the reference `checkImageReadings` compares
that lookup with, on every spelling. The WORDS rules both need are written once
and shared (`helpers/adverb_of.hpp`).

## Why reflection?

Reading Whitaker's WORDS from C++ is a problem I keep coming back to, as a place
to try out new things I've learned. With reflection such an exciting addition
to C++, it was natural for this project to start asking what it could do here.
The first use was replacing magic_enum for turning an `enum class` into a
`string_view`. The first public version, 0.1, was already built this way.
Since then, a few places have shown where it really shines.

- **Columns on the struct.** DICTLINE's fixed columns are annotations on the
  record's members (`[[= Column{0, 19}]] Stem stem1;`), and the parser reads
  them from there.
- **Records filled member by member.** A `template for` over a struct's members
  parses each field into the next member, so a new member needs no parser
  change.
- **Source tokens as tags.** Each enumerator carries WORDS' token
  (`nominative LATIN_WORDS("NOM")`), and `N`, `ADJ`, `TRANS` are read and
  printed from the tags, with no name tables.
- **Types know their part of speech.** `struct [[= Part::noun]] Noun`, read by
  `latin::partOf`, in place of hand-written switches.
- **One index type per source file.** With `source::Index<^^scheme::dictline>`,
  a DICTLINE index can't be passed where an INFLECTS one is expected.
- **Any value described.** Error reports print a record field by field, and
  name exactly the fields that differ.
- **Bindings from tags.** The C++ header marks its calls and fields
  `WHITAKER_TAG(bind)`, and gen reads the tags to write the C header, the Python
  module and the inspector's API view, and refuses to build while any of them
  is stale.
- **Steps that schedule themselves.** Gen's steps are found, ordered and run
  in parallel from their declarations alone (Pipeline, above).

0.4.0 changed how gen is put together, not what its steps compute, and
reflection now runs through all of it. Some of it I'm proud of. Some of it
might look a bit cursed, like the execution steps, with annotations used as a
dependency ledger and a scheduler worked out at compile time.

The cost is GCC 16 only, and `-Wno-shadow`, since every `template for` trips
`-Wshadow`. Editors lag behind. clangd only gets close with the
[clang-p2996](https://github.com/bloomberg/clang-p2996) fork, and until my fix
([#361](https://github.com/bloomberg/clang-p2996/pull/361)) is in, it trips
over annotations. clangd keeps the headers it has parsed as a precompiled
header, and the fork doesn't save annotation values in one, so they come back
empty. With the fix, clangd is clean on gen except `src/types/latin_test.cpp`,
where the fork rejects an enumerator reflection passed to `substitute` (GCC
accepts it). The library itself is C++23 and has no reflection. Only `gen`
and the oracle's probe use it.

## Checks

- **Build.** Latin declarations fit their image and reading slots, and the
  generated public companions are current.
- **Load.** The image loads as the library would (header, directory, section
  sizes).
- **Spellings.** Every kept spelling, in order, as the loaded image lists them
  (`rel::spellings()`).
- **Rows.** Every row of each spelling, in order, with its stem, meaning and
  grammar.
- **Entries.** All 39,415 stored entries match the parsed DICTLINE and UNIQUES
  words.
- **Addon search.** The records only the addon search reads stay inside their
  tables, and the endings and stems are sorted. Which rows they hold is checked
  by the oracle, against WORDS' own readings, not here.

All but the first run on a temporary file beside the image
(`data/.whitaker.dat.tmp.<n>`). Only an image that passes every check replaces
`data/whitaker.dat`. The first failure stops `gen_image`, the old image is
untouched, and the failed one stays for inspection.

## Getting WORDS' files

Gen reads WORDS' four files from `data/source/`. They are not shipped, only
their hashes in `data/source/MANIFEST.sha256` and their licence. The library
itself never reads them, since it builds from the checked-in
`data/whitaker.dat`. To regenerate the image, fetch them at the upstream
revision the root README names under "Data and licence" (`1f2f0fb0867a` of
`mk270/whitakers-words`).

```sh
git clone https://github.com/mk270/whitakers-words /tmp/whitakers-words
git -C /tmp/whitakers-words checkout 1f2f0fb0867a
cp /tmp/whitakers-words/{DICTLINE.GEN,INFLECTS.LAT,UNIQUES.LAT,ADDONS.LAT} data/source/
(cd data/source && sha256sum -c MANIFEST.sha256)
```

## Updating the corpus

To try a newer WORDS, follow these steps.

1. Fetch the newer files into `data/source/` and update their hashes in
   `data/source/MANIFEST.sha256`. Note the upstream revision in the root
   README under "Data and licence" and in "Getting WORDS' files" above.
2. Update the counts in the schemes under `src/source/`, and in Inputs above.
3. Build the `image` target, which replaces `data/whitaker.dat` only when
   every check passes. Then build the library and run its tests.

The checks prove the image holds exactly what expansion built. They do not
prove a corpus change is linguistically right, nor that every WORDS front-end
behaviour is reproduced.
