# whitakerLib lookup-image generator

This maintainer tool builds `../data/whitaker.dat`, the lookup image embedded
by whitakerLib. The image is checked in, so library users neither build nor run
the generator.

The generator is intentionally narrow. It understands the pinned WORDS source
layout used by this repository and writes one fixed output. It is not a
general-purpose dictionary converter and it has no command-line options.

## Inputs and output

Every run reads:

```text
../data/source/DICTLINE.GEN
../data/source/INFLECTS.LAT
../data/source/UNIQUES.LAT
../data/source/ADDONS.LAT
```

and writes:

```text
../data/whitaker.dat
```

The paths and source formats are compiled in. The program works from any
directory, but alternate inputs, outputs, or formats require a code change.
Each source's line and record counts are stated in its scheme under
`src/source/wl_stable/`, and a file that does not match them is refused:
`DICTLINE.GEN` 39,335 lines, `INFLECTS.LAT` 3,228 lines with 1,797 records,
`UNIQUES.LAT` 237 lines with 79 records, `ADDONS.LAT` 1,200 lines with 343
records.

## Toolchain

The generator currently requires:

- GNU/Linux;
- CMake 3.28 or newer; and
- GCC 16 with C++26 reflection support and `-freflection`.

This toolchain is needed only to regenerate the image.

## Build and test

Run these commands from the parent `whitakerLib` directory:

```sh
cmake -S codegen -B build-codegen -DCMAKE_BUILD_TYPE=Release
cmake --build build-codegen -j2
ctest --test-dir build-codegen --output-on-failure
```

The two CTest executables check the compile-time stage roster. Corpus
validation runs in `gen`, where the complete data is available.

## Generate the image

```sh
./build-codegen/gen
```

The command reports each stage, replaces the tracked image after structural
validation, and then performs its exhaustive comparison. Review the result
before committing it:

```sh
sha256sum data/whitaker.dat
git diff --stat -- data/whitaker.dat
```

For the image currently checked in:

```text
size:    10,768,086 bytes
SHA-256: 72bd888df04fff026a5c817abc64f1d62914f89205cba683d982f19949e68820
```

An unchanged source and unchanged generator are expected to produce the same
image byte for byte.

## Measured generation time

A warmed Release run took a median 3.84 seconds on an AMD Ryzen 5 5600X with
GCC 16.2.1. This includes parsing, construction, publication, structural
validation, and exhaustive post-write comparison, but not compilation. It is
a single-machine baseline, not a guarantee.

## Pipeline

One run has four phases:

1. Construction reads the fixed-width source files into validated records.
2. Tokenization converts source fields into typed values and shared strings.
3. Expansion applies the WORDS morphology rules and decides which generated
   forms and readings are retained.
4. Emission builds the lookup state machine and relationship tables, then
   serializes them as `whitaker.dat`.

The phases use one in-memory board. Each value has one writer; later phases
receive it read-only. Compile-time checks require a complete, unique stage and
hook set.

## Publication and validation

The generator does not write directly over the existing image. It creates a
temporary sibling file, flushes it to storage, closes it, and asks the runtime
image reader to validate its structure. Only then does it rename the temporary
file over `data/whitaker.dat`.

After publication, the generator reopens the image and checks:

- the header, section directory, state machine, instructions, records, string
  pool, and complete graph walk;
- every spelling retained by expansion;
- every ordered analysis row for each spelling;
- all four rendered fields: stem, meaning, part of speech, and inflection;
- the stated word-length and row counts; and
- rejection of representative corruptions to the image format.

The semantic comparison happens after replacement. If it fails, the new image
remains for inspection, but the command exits with an error.

## Updating the corpus

When the vendored WORDS files change:

1. Record the upstream revision and update the files under `data/source/`.
2. Update `data/source/MANIFEST.sha256` with the reviewed source hashes.
3. Build and run the generator.
4. Confirm that every generator check passes.
5. Build and test the public library against the new image.
6. Review behavior changes before committing the sources and image together.

The generator proves that the image represents its constructed rows. It does
not prove a corpus change is linguistically correct or reproduce every WORDS
front-end behavior.
