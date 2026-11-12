#include <whitaker.hpp>
#include <cassert>
#include <cstring>
#include <string>

int main(const int argc, char**) {
  using namespace whitaker;
#ifdef __EMSCRIPTEN__
  if (argc > 1) {
    Result bounded;
    std::size_t count = 0;
    Status status;
    while ((status = append(bounded, "arma")) == Status::ok)
      ++count;
    assert(status == Status::outOfMemory && count != 0);
    assert(wordCount(bounded) == count && readings(bounded).size() == 4 * count);
    const WordId last{count - 1};
    assert(decode(bounded, last, readings(bounded, last).back()).dictionaryWordId == DictionaryWordId{4769});
    clear(bounded);
    assert(append(bounded, "arma") == Status::ok && readings(bounded).size() == 4);
    return 0;
  }
#else
  (void)argc;
#endif
  static_assert(sizeof(Reading) == 8);
  static_assert(sizeof(WordOffsets) == 2 * sizeof(std::size_t));
  Result result{};
  assert(analyze(result, "arma") == Status::ok);
  assert(wordCount(result) == 1 && readings(result).size() == 4);
  ReadingDetail d = detail(result, WordId{0}, 0);
  assert(d.dictionaryWordId == DictionaryWordId{4768} && std::strcmp(d.form.stemSpelling, "arm") == 0 &&
         std::strcmp(d.form.ending, "a") == 0);
  assert(d.form.grammar.part == latin::Part::N && d.form.grammar.caseOf == latin::Case::NOM);
  assert(append(result, "ZZZZ") == Status::ok);
  assert(append(result, "amasti") == Status::ok);
  assert(append(result, "MMMM") == Status::ok);
  std::size_t rowCount = 0, readingTotal = 0, unknown = 0;
  for (const Row& row : rows(result)) {
    ++rowCount;
    if (row.reading)
      ++readingTotal;
    else
      ++unknown;
  }
  assert(readingTotal == readings(result).size() && unknown == 1 && rowCount == readingTotal + unknown);
  auto copy = clone(result);
  assert(copy);
  clear(result);
  assert(wordCount(result) == 0 && wordCount(*copy) == 4);
  d = detail(*copy, WordId{2}, 0);
  assert(std::strcmp(d.form.stemSpelling, "amav") == 0 && d.form.derivation.syncope != latin::Syncope{});
  d = detail(*copy, WordId{3}, 0);
  assert(d.dictionaryWordId == kNoDictionaryWord && d.dictionaryWord.numeralValue.value == 4000);
  const auto saved = d;
  assert(std::strcmp(saved.form.stemSpelling, "MMMM") == 0);
  assert(append(*copy, std::string(kMaxWordLength + 1, 'a')) == Status::inputTooLong && wordCount(*copy) == 4);
  assert(analyze(result, "") == Status::ok && spelling(result, WordId{0}).empty() && readings(result).empty());
  Result moved = std::move(*copy);
  assert(wordCount(moved) == 4);
  assert(append(result, spelling(moved, WordId{0})) == Status::ok && readings(result).size() == 4);
  clear(result);
  for (unsigned i = 0; i < 1000; ++i)
    assert(append(result, i % 2 ? "amasti" : "arma") == Status::ok);
  for (std::size_t i = 0; i < wordCount(result); ++i)
    assert(std::strcmp(detail(result, WordId{i}, 0).form.stemSpelling, i % 2 ? "amav" : "arm") == 0);
  assert(detail(result, WordId{wordCount(result)}, 0).dictionaryWordId == DictionaryWordId{} &&
         readingCount(result, WordId{5000}) == 0);
  std::size_t at = 0;
  for (const Row& row : rows(result)) {
    assert(row.reading && row.reading->entry == readings(result)[at].entry);
    ++at;
  }
  assert(at == readings(result).size());
  assert(std::strcmp(kVersion, "0.4.0") == 0 && imageFormat() == 11);
  assert(std::strlen(imageChecksum()) == 64);
}
