# whitakerLib

[![CI](https://github.com/ddd2951/whitakerLib/actions/workflows/ci.yml/badge.svg)](https://github.com/ddd2951/whitakerLib/actions/workflows/ci.yml)

A small C++23 library that takes a Latin word and returns every reading William
Whitaker's WORDS data supports, with its stem, meaning, part of speech and
inflection, add-ons and Roman numerals included. With a C binding and a Python binding,
both generated from the C++ header.

- **Under a microsecond a word**, on average over [102 Latin books](data/booklist.md#books).
- **1,205,496 spellings, 2,374,470 readings** in a 3.5 MB image embedded in
  the library, with no external data file at runtime, no database and no
  initialization.
- **99.58% of words answered exactly as the Ada WORDS answers them**, every
  reading, record and add-on, in WORDS' order. The [oracle](oracle/README.md)
  explains the comparison.
- **[Try it in your browser](https://ddd2951.github.io/whitakerLib/)** and see every reading of a text,
  as the API returns it and as the lookup's trace.

## Why?

whitakerLib started as my own training project. I made it public because I
couldn't find anything that worked like this, with fast lookups, no
dependencies, low memory usage and a compact API.

It isn't meant as an easy tool for translating Latin, but as the backend for
other things, such as running it on a Kindle or another low-resource device.

I'm no Latin expert. I come at this mostly from the programming side, and I
don't think I've figured out anything new. What I have found is that Whitaker
built something that really works. The more I've worked with WORDS, the more
I've come to appreciate how he approached Latin, and his data is genuinely nice
to work with. I think his ideas go much further than what the program itself
shows today.

I can't help wondering how far the same approach could be taken with other
languages that follow similar patterns, and how much of what makes WORDS work is
specific to Latin, rather than to the kind of data that fits this representation
in the first place.

Most Latin tools won't need lookups this quick and the generator could have been
written without reflection, but both were fun to build. The reasons why are
given in "Why reflection?" in `codegen/README.md`.

## The inspector

A page I built to test the library and find its bottlenecks, shipped so anyone
can see what the library does in a browser. Paste some Latin and move through
every reading, as the API returns it and as the lookup's trace. It's a
debugging and exploration tool, not primarily a tool for reading Latin, even if
it works for that. A rewrite of the page is planned, to try a few different
things with it, so don't build anything that depends on it.

It's online at [ddd2951.github.io/whitakerLib](https://ddd2951.github.io/whitakerLib/), rebuilt from `main` on
every push. It runs fully in the page, the library compiled to WebAssembly, and
sends nothing. It's a 3.8 MB download, about 1.2–1.6 MB over the network depending on
the server's compression. It needs WebAssembly, so browsers such as the Kindle
Paperwhite's can't run it.

- **Walking through a text.** ← → (or a click) moves word by word, and the panel
  shows that word. Drag or Shift-select several words to see each spelling once,
  with how often it occurs. `1`–`4` switch the views below.
- **Readings.** Each reading in words, with its meaning, grammar, stem and
  ending, and add-ons.
- **API.** What your code gets for that word, every field by its name, in C++,
  C or Python (`b` switches).
- **Trace.** Why the word got the readings it did. First the lookup's passes in
  the order they run (as in [Inside](#inside)), each marked ran or not run, with
  how many readings it added. Some passes run only when the earlier ones found
  nothing. Then each reading with the pass that found it, its route (direct,
  syncope, adverb, slur, word mod), its stem key, and its add-ons, outermost
  first.
- **Statistics.** For the selection, how fast the library looked it up,
  readings per spelling, routes, and the words with no reading.
- **Search** (`/`). Grammar (`abl pl`), a route (`syncope`), `unknown` for words
  with no reading, or part of a spelling. ↑ ↓ step through the matches.

To build and serve it yourself.

```sh
source ~/emsdk/emsdk_env.sh
emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm
python3 -m http.server -d build-wasm/site
```

## Arma virumque cano

The first three words of the *Aeneid*. This is how the library reads them.

- **arma** is "arms, weapons", as nominative, vocative or accusative plural,
  or the imperative of *armo*, "arm!"
- **virumque** loses the tackon *-que*, "and", leaving *virum*, read as *vir*,
  "man, hero", alongside *virus* and the old genitive plural of
  *vis*, "strength".
- **cano** is "I sing", but also the dative or ablative of *canus*, "gray hair,
  old age", of *canum*, "wicker basket", and of the adjective *canus*,
  "white, hoary".

So as far as single words go, Virgil might be singing of arms and the man, or
telling you to arm the man for the wicker basket. Or for old age. Or for the
white one. *cano* won't say which. The library gives you every reading.

Choosing the meaning is the reader's part, as it always was.

## API

The project is pre-1.0 and its ABI is not frozen, but until 0.5.0 nothing in the
C++ API changes or goes away. New calls and fields may be added.
`include/whitaker.hpp` is C++23, and it opens with what every call promises.

```cpp
whitaker::Result result;
if (whitaker::analyze(result, "amoque") != whitaker::Status::ok)
  return 1;
const whitaker::WordId word{0};
for (std::size_t r = 0; r < whitaker::readingCount(result, word); ++r) {
  const whitaker::ReadingDetail detail = whitaker::detail(result, word, r);
  // detail.dictionaryWord, detail.meaning, detail.form.grammar, detail.form.stemSpelling, ...
}
```

The data is plain data and the functions are free functions. `Result` is the one
owner, holding a batch's words and readings, and everything else reads it.

- `analyze()` replaces the result, and `append()` adds one word. Unknown words
  return `ok` with no readings, words longer than 24 bytes return `inputTooLong`,
  and when memory runs out, `outOfMemory`. `clear()` keeps the capacity.
- A word is a `WordId`, its place in the result. `spelling()`, `trimmed()`,
  `readings()` and `readingCount()` take one, and past the last word they give
  the empty value.
- `detail(result, word, reading)` decodes one reading into its `dictionaryWord` (what
  the dictionary says of the word, which no ending changes), its `form` (stem,
  ending and grammar) and its meaning and add-ons. `decode()` does the same for a
  reading you already hold.
- A reading is stored as 8 bytes of plain data (`latin::packed::Reading`, what
  `readings()` gives), and `detail()` decodes one into a `ReadingDetail` (160 bytes
  today) only when you ask. The result keeps the 8 bytes, plus a change record
  for a reading that has one (syncope, add-ons, a respelling), however many
  fields you read.
- `rows(result)` is the whole batch as a table, one row per reading, a standard
  view over the result.
- What a call lends goes stale on the next `analyze`, `append` or `clear`, and
  strings returned as `const char*` belong to the library. Different results run on
  different threads in parallel.

The generated `<latin_generated.hpp>` companion gives Latin names and grammar
descriptions, and `addonSpelling()`, `addonMeaning()` and `spellingRule()`
describe the ids a reading carries.

### C and Python

Both are generated from the C++ header's `WHITAKER_TAG(bind)` tags by `codegen`,
with the same fields by the same names, and the build refuses to run while either
is out of date with the header. For now, the inspector is the place to see them.
Its API view shows every call and every field of a reading as C++, C or Python.
Neither binding is settled until 0.5.0. Unlike the C++ API, the C and Python
bindings may still change. There is no WebAssembly binding yet, since the
inspector builds the library to WebAssembly for itself.

## What it does, and doesn't

- Returns every reading, in WORDS' order, with WORDS' add-ons (prefixes,
  suffixes, tackons, packons and tickons).
- Syncopated perfects read as their full forms, as in WORDS' main syncope
  pass, so `amasti` reads as `amavisti`. `form.derivation.syncope` says which rule is used.
- Case-insensitive. `j` folds with `i` and `v` with `u`, except in Roman
  numerals and, as in WORDS, in the syncope pass. ASCII only.
- `form.stemSpelling` is the matched stem and `form.ending` the inflection after it
  (`am` + `o`), not a dictionary headword.
- One word per call, with no tokenizing, parsing, ranking or translation.
- WORDS' word-split and bad Roman numeral guesses are not implemented.
- Not a drop-in replacement for WORDS. It has the same data and follows the same
  lookup rules closely, but it isn't identical for every input.

## Speed and accuracy

Both were measured over the same [102 Latin books](data/booklist.md#books),
totaling 4,662,505 word occurrences.

**Speed.** On one thread, the C++ example (`examples/cpp`) analyzes the
4,662,505 words, then decodes and describes every one of their 17,460,251
readings, in 3.53 s, or 0.76 µs a word. On the CPU's 12 threads it takes 0.49 s.
Reading the files and splitting them into words is not timed. Finding a word's
readings alone, without decoding them, takes 421 ns a word natively and 610 ns
in WebAssembly under Node. Ryzen 5 5600X, GCC 16, a Release build.

**On a Kindle.** The library is tested and confirmed working on a Kindle
Paperwhite 3 (Cortex-A9, about 1 GHz), cross-compiled with Clang as a Release
build targeted at its CPU (`-O3 -mcpu=cortex-a9 -mfpu=neon`), with no other
tuning. On 200,000 words evenly spaced through the books, it looks up each word
and decodes every reading in 2.56 s on one thread, 12.8 µs a word. The lookup
alone takes 8.8 µs.
It returns the same 750,215 readings as the PC. The KOReader plugin comes in
0.4.1.

**Accuracy.** Every word is compared with the answer from the Ada WORDS
program, built using the same data, down to each reading with its stem, each
dictionary record and meaning, the add-ons, and the order of all of them.

**99.58%** of the words (97.35% of the distinct spellings) get exactly WORDS'
answer. The [book list](data/booklist.md#books) gives the per-book results, and
the [oracle README](oracle/README.md#where-we-are-now) explains the remaining
differences. That comparison uses the patched WORDS revision `5dd8ca5`,
described in the [oracle README](oracle/README.md#bugs-found-in-words).

Tracking down where the two disagree turned up several bugs in WORDS itself,
some of which crashed it. Several of the fixes have been
[merged upstream](https://github.com/mk270/whitakers-words/pulls?q=is%3Apr+author%3Addd2951),
some are waiting for review, and others I still need to submit.

## Inside

The steps a word goes through, in WORDS' order.

```text
analyze(result, "adiitque")
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

The dictionary lookup itself is done ahead of time. `codegen` runs it over
every stem and ending and stores the answers in the image
(`codegen/README.md`).

For readers of the Ada source, where each WORDS procedure lives.

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

- **0.4.1.** Some small updates, and a KOReader plugin (Kindle, Kobo,
  PocketBook, Android) in a repository of its own, as a standalone tool. Long-press
  a Latin word for every reading, offline.
- **Later.** A rewrite, with the lookup itself generated from one description.

## Build

Tested on Linux only. It needs CMake 3.21, GCC 15 or Clang 20 (for `#embed`
in C++, with its offset parameter). Callers use C++23 and `whitaker.hpp`, or C11
and `whitaker.h`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
cmake --install build --prefix /your/prefix
```

The options are `WHITAKER_SANITIZE` (ASan and UBSan), `WHITAKER_BUILD_PYTHON` (needs
pybind11), and `WHITAKER_BUILD_TESTING`, `WHITAKER_INSTALL` and `WHITAKER_WERROR`,
which are on when whitakerLib is the top-level project and off under
`add_subdirectory()`.

The install provides a CMake package and a relocatable `whitaker.pc`.

```cmake
find_package(Whitaker CONFIG REQUIRED)
target_link_libraries(your_target PRIVATE Whitaker::whitaker)
```

`bindings/c` and `bindings/python` are generated from the header, and
`inspector/` is built with Emscripten (above). `examples/cpp` reads on several
threads and compares them with one thread, and `examples/c` prints every reading
of the words given. ctest runs one C++
and one C program over the public API.

## Contributing

It isn't an easy project to jump into yet, but I want it to become one people
can join. From 0.4.0 on, the work goes in as many small commits instead of a
few big ones, which should make it easier to follow.

**Words.** If the library reads a word from a book differently than WORDS
does, open an issue or a pull request. A link to the book, or a picture of the
page with the word on it, is enough.

**Code.** Anything that makes it faster or smaller is welcome, as long as I
can understand it. I'm not strict on style either. If you know a cool trick,
show me. How you got there doesn't matter, AI included, as long as you
understand the change you're sending.

## Data and licence

The data is from Colonel William Whitaker's WORDS, revision `1f2f0fb0867a`
(2026-08-26) of
[`mk270/whitakers-words`](https://github.com/mk270/whitakers-words), the files
`DICTLINE.GEN`, `INFLECTS.LAT`, `UNIQUES.LAT` and `ADDONS.LAT`, unmodified.
They are not shipped. `data/source/MANIFEST.sha256` holds their hashes and
`codegen/README.md` says how to fetch them, which only regenerating the image needs.

`data/whitaker.dat` is image format 11, 3,473,257 bytes, SHA-256
`8251ea4fb298a1b28e0fe978759d7b41a3cb84216fbc95cfc27a7d001d490d97`.
`codegen/README.md` says how it is made and checked. It is checked in, so a
build cannot change it silently.

The library is under the European Union Public Licence v. 1.2 (`LICENSE`).
The WORDS data is a separate work under Whitaker's notice
(`data/source/LICENCE.txt`).

A big thanks to William Whitaker for making his program available. Not all of
it has aged like fine wine, but I think the systems underneath it make sense
for Latin.

And an equally big thanks to [mk270](https://github.com/mk270) for keeping it
well maintained, and for making sure a program of that age still moves forward.
Writing a program is one thing. Understanding someone else's well enough to
carry it forward, with respect for what they built, is another, and it doesn't
get the credit it deserves.
