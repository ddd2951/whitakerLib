#pragma once

// INFO: The library's public API. C++23.
//  The inspector shows the C++, C and Python APIs and is the recommended place to read and understand them.
//
//  What every call here promises, so the declarations below only say where they differ:
//  - Data is plain data: structs of values you may copy freely. The one owner is Result; the functions read it.
//  - Ids (a dictionary word's, an add-on's, a spelling rule's) index this build's dictionary image: valid only with the
//    library that gave them.
//  - A WordId is a word's place in its Result, valid only in that Result.
//  - What a call lends (a span, a spelling, rows) lives in the Result and goes stale on its next analyze, append or
//    clear. Strings returned as const char* belong to the library and live as long as the program.
//  - Past the last word or reading, and for an unknown id, a call gives the empty value: "", 0, false, null or a
//    zeroed struct. It never reads out of range.
//  - No call throws. Only analyze, append and clone allocate; when memory runs out they say so (OutOfMemory, or an
//    empty clone).
//  - Calls on different Results run in parallel. One Result may be read by any number of threads at once, but not
//    while one of them analyzes, appends or clears it. The dictionary is read-only and shared; each thread's lookup
//    scratch is its own.

#include <latin_generated.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

#define WHITAKER_TAG(name) WHITAKER_TAG_##name

#if defined(_WIN32) && defined(WHITAKER_BUILDING)
#define WHITAKER_TAG_api __declspec(dllexport)
#elif defined(_WIN32)
#define WHITAKER_TAG_api __declspec(dllimport)
#elif defined(__GNUC__)
#define WHITAKER_TAG_api __attribute__((visibility("default")))
#else
#define WHITAKER_TAG_api
#endif

// INFO: The tags gen reads to write the bindings.
// NOTE: Without reflection the binding tags are empty, so this header stays C++23.
#ifdef __cpp_impl_reflection
namespace whitaker::tag {
struct Bind {};
struct Hide {};
inline constexpr Bind bind;
inline constexpr Hide hide;
} // namespace whitaker::tag
#define WHITAKER_TAG_bind [[= ::whitaker::tag::bind]]
#define WHITAKER_TAG_hide [[= ::whitaker::tag::hide]]
#else
#define WHITAKER_TAG_bind
#define WHITAKER_TAG_hide
#endif

namespace whitaker {

inline constexpr char kVersion[] = "0.4.0";
inline constexpr std::size_t kMaxWordLength = 24;

enum class Status : unsigned char {
  ok,
  invalidArgument,
  inputTooLong,
  outOfMemory,
};

struct AddonStep {
  std::uint16_t id{};
  latin::AddonKind kind{};
};
static_assert(std::is_trivially_copyable_v<AddonStep>);

// INFO: DICTLINE's words first, then UNIQUES', from kFirstUnique.
enum class DictionaryWordId : std::uint16_t {};
inline constexpr DictionaryWordId kFirstUnique{39336};
inline constexpr DictionaryWordId kNoDictionaryWord{0xFFFF};

// INFO: How a reading was reached besides the dictionary: a restored syncope, an adverb made from an adjective.
struct Derivation {
  latin::Syncope syncope : 3;
  bool adverb : 1;
};
static_assert(sizeof(Derivation) == 1);
static_assert(std::is_trivially_copyable_v<Derivation>);

// INFO: The stem spelling and inflection for a reading.
//  ending is empty when absent; spellingRuleId is 0 when no spelling rule was used.
struct Form {
  char stemSpelling[kMaxWordLength + 1]{};
  char ending[kMaxWordLength + 1]{};
  latin::Analysis grammar{};
  latin::Age inflectionAge{};
  latin::Frequency inflectionFrequency{};
  Derivation derivation{};
  std::uint8_t spellingRuleId{};
};
static_assert(std::is_trivially_copyable_v<Form>);

enum class WordId : std::size_t {};

struct WordOffsets {
  std::size_t firstReading{};
  std::size_t firstData{};
};

static_assert(std::is_trivially_copyable_v<WordOffsets>);

struct SpellingRule {
  const char* first{};
  const char* second{};
  bool reversed{};
  bool slur{};
  latin::SpellingKind kind{};
};
static_assert(std::is_trivially_copyable_v<SpellingRule>);

// INFO: One reading, packed; detail() or decode() unpacks it.
using Reading = latin::packed::Reading;
static_assert(sizeof(Reading) == 8);
static_assert(std::is_trivially_copyable_v<Reading>);

// INFO: One reading, decoded. For a Roman numeral, meaning is "as a ROMAN NUMERAL;", dictionaryWord.numeralValue
//  holds the value, and dictionaryWordId is kNoDictionaryWord.
struct ReadingDetail {
  latin::DictionaryWord dictionaryWord{};
  DictionaryWordId dictionaryWordId{};
  Form form{};
  const char* meaning{};
  const char* partName{};
  AddonStep addons[kMaxWordLength / 2]{};
  std::uint8_t addonCount{};
  latin::Route route{};
  // INFO: Stem key from INFLECTS, 1 to 4; 0 when unavailable.
  std::uint8_t stemKey{};
};

static_assert(std::is_trivially_copyable_v<ReadingDetail>);

// INFO: One row of a batch: one per reading, or one with no reading for a word that has none.
struct Row {
  WordId word{};
  std::string_view spelling{};
  bool trimmed{};
  std::optional<Reading> reading{};
};
static_assert(std::is_trivially_copyable_v<Row>);

// INFO: Owns the words and readings; allocates on first use.
class Result {
public:
  Result() noexcept = default;
  WHITAKER_TAG(api) ~Result();
  Result(Result&& other) noexcept;
  Result& operator=(Result&& other) noexcept;

  struct Storage;

  WordOffsets* words = nullptr;
  Reading* readings = nullptr;
  unsigned char* data = nullptr;
  std::size_t wordCount = 0;
  std::size_t readingCount = 0;
  Storage* storage = nullptr;
};

inline void swap(Result& a, Result& b) noexcept {
  std::swap(a.words, b.words);
  std::swap(a.readings, b.readings);
  std::swap(a.data, b.data);
  std::swap(a.wordCount, b.wordCount);
  std::swap(a.readingCount, b.readingCount);
  std::swap(a.storage, b.storage);
}

inline Result::Result(Result&& other) noexcept { swap(*this, other); }

inline Result& Result::operator=(Result&& other) noexcept {
  swap(*this, other);
  return *this;
}

static_assert(!std::is_copy_constructible_v<Result>);

// NOTE: Plain arguments: no standard library type crosses the shared library's boundary.
namespace boundary {
// INFO: A word's first data byte: its spelling's size, and whether its readings were trimmed.
inline constexpr unsigned char kSpellingSizeMask = 31;
inline constexpr unsigned char kTrimmedFlag = 32;
static_assert(kMaxWordLength <= kSpellingSizeMask);
[[nodiscard]] WHITAKER_TAG(api) Status read(Result& result, const char* word, std::size_t size, bool replace) noexcept;
[[nodiscard]] WHITAKER_TAG(api) bool cloneInto(const Result& result, Result& copy) noexcept;
} // namespace boundary

// INFO: Replaces the result's words with this one. Unknown words return Ok with no readings.
WHITAKER_TAG(bind) [[nodiscard]] inline Status analyze(Result& result, std::string_view word) noexcept {
  return boundary::read(result, word.data(), word.size(), true);
}

// INFO: Adds the word after the result's words.
WHITAKER_TAG(bind) [[nodiscard]] inline Status append(Result& result, std::string_view word) noexcept {
  return boundary::read(result, word.data(), word.size(), false);
}

// INFO: Removes all words without freeing the allocated memory.
WHITAKER_TAG(bind) WHITAKER_TAG(api) void clear(Result& result) noexcept;

// INFO: An independent copy.
[[nodiscard]] inline std::optional<Result> clone(const Result& result) noexcept {
  Result copy;
  return boundary::cloneInto(result, copy) ? std::optional<Result>{std::move(copy)} : std::nullopt;
}

WHITAKER_TAG(bind) [[nodiscard]] inline std::size_t wordCount(const Result& result) noexcept {
  return result.wordCount;
}

// INFO: The word as it was given, NUL-terminated.
WHITAKER_TAG(bind) [[nodiscard]] inline std::string_view spelling(const Result& result, WordId word) noexcept {
  const auto w = std::to_underlying(word);
  if (w >= result.wordCount)
    return {};
  const auto* data = result.data + result.words[w].firstData;
  return {reinterpret_cast<const char*>(data + 1), std::size_t(*data & boundary::kSpellingSizeMask)};
}

// INFO: Whether rarer readings of the word were left out, as WORDS does.
WHITAKER_TAG(bind) [[nodiscard]] inline bool trimmed(const Result& result, WordId word) noexcept {
  const auto w = std::to_underlying(word);
  return w < result.wordCount && (result.data[result.words[w].firstData] & boundary::kTrimmedFlag) != 0;
}

[[nodiscard]] inline std::span<const Reading> readings(const Result& result, WordId word) noexcept {
  const auto w = std::to_underlying(word);
  if (w >= result.wordCount)
    return {};
  const auto first = result.words[w].firstReading;
  const auto end = w + 1 < result.wordCount ? result.words[w + 1].firstReading : result.readingCount;
  return {result.readings + first, end - first};
}

// INFO: Every reading of every word, in order.
[[nodiscard]] inline std::span<const Reading> readings(const Result& result) noexcept {
  return {result.readings, result.readingCount};
}

WHITAKER_TAG(bind) [[nodiscard]] inline std::size_t readingCount(const Result& result, WordId word) noexcept {
  return readings(result, word).size();
}

// INFO: A reading of the word, decoded.
[[nodiscard]] WHITAKER_TAG(api) ReadingDetail decode(const Result& result, WordId word,
                                                     const Reading& reading) noexcept;

// INFO: Reading `readingIndex` of the word, decoded.
WHITAKER_TAG(bind) [[nodiscard]] inline ReadingDetail detail(const Result& result, WordId word,
                                                             std::size_t readingIndex) noexcept {
  const auto all = readings(result, word);
  return readingIndex < all.size() ? decode(result, word, all[readingIndex]) : ReadingDetail{};
}

// INFO: The whole result as a table of Rows, in order.
WHITAKER_TAG(bind) [[nodiscard]] inline auto rows(const Result& result) noexcept {
  return std::views::iota(std::size_t{0}, result.wordCount) | std::views::transform([&result](std::size_t w) {
           const WordId word{w};
           const auto all = readings(result, word);
           return std::views::iota(std::size_t{0}, std::max<std::size_t>(all.size(), 1)) |
                  std::views::transform(
                      [word, all, text = spelling(result, word), cut = trimmed(result, word)](std::size_t r) {
                        return Row{word, text, cut, r < all.size() ? std::optional{all[r]} : std::nullopt};
                      });
         }) |
         std::views::join;
}

[[nodiscard]] WHITAKER_TAG(api) unsigned imageFormat() noexcept;
[[nodiscard]] WHITAKER_TAG(api) const char* imageChecksum() noexcept;

WHITAKER_TAG(bind) [[nodiscard]] WHITAKER_TAG(api) const char* addonSpelling(std::uint16_t id) noexcept;
WHITAKER_TAG(bind) [[nodiscard]] WHITAKER_TAG(api) const char* addonMeaning(std::uint16_t id) noexcept;

// INFO: Zeroed for rule 0, which is none.
WHITAKER_TAG(bind) [[nodiscard]] WHITAKER_TAG(api) SpellingRule spellingRule(std::uint8_t rule) noexcept;

} // namespace whitaker

#undef WHITAKER_TAG
#undef WHITAKER_TAG_api
#undef WHITAKER_TAG_bind
#undef WHITAKER_TAG_hide
