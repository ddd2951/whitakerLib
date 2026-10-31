// INFO: Compare threaded and sequential lookup fingerprints. Non-letters separate words;
//  with no files, use three opening lines.
//
//   whitaker-cpp-example caesar.txt virgil.txt ...
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <future>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>
#include <string>
#include <thread>
#include <vector>

#include <whitaker.h>

namespace {

constexpr std::size_t kBatch = 1024;

// INFO: FNV-1a fingerprint of each reading's stem and grammar.
constexpr std::uint64_t kFnvBasis = 14695981039346656037u;
constexpr std::uint64_t kFnvPrime = 1099511628211u;

struct Tally {
  long readings = 0;
  std::uint64_t fingerprint = kFnvBasis;
  std::size_t busiest = 0;
  std::size_t busiestCount = 0;
};

using Result = std::unique_ptr<WhitakerResult, decltype(&whitaker_result_free)>;

Result newResult() {
  Result result{whitaker_result_new(), whitaker_result_free};
  if (!result)
    throw std::bad_alloc{};
  return result;
}

void fold(std::uint64_t& hash, const char* text) {
  for (; *text != '\0'; ++text)
    hash = (hash ^ static_cast<unsigned char>(*text)) * kFnvPrime;
  hash = (hash ^ 0xff) * kFnvPrime;
}

std::vector<std::string> wordsOf(const std::string& text) {
  std::vector<std::string> words;
  std::string word;
  for (const char c : text) {
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
      word += c;
    } else if (!word.empty()) {
      words.push_back(word);
      word.clear();
    }
  }
  if (!word.empty())
    words.push_back(word);
  return words;
}

// NOTE: Each call owns its result and writes a separate tally.
void analyzeBatch(const std::vector<std::string>& words, std::vector<Tally>& tallies, std::size_t k) {
  const Result result = newResult();
  Tally tally;
  const std::size_t end = std::min(words.size(), (k + 1) * kBatch);
  for (std::size_t i = k * kBatch; i < end; ++i) {
    const auto status = whitaker_analyze(words[i].c_str(), result.get());
    if (status == WHITAKER_OUT_OF_MEMORY)
      throw std::bad_alloc{};
    if (status != WHITAKER_OK)
      continue;
    tally.readings += static_cast<long>(result->form_count);
    for (std::size_t f = 0; f < result->form_count; ++f) {
      char line[64];
      whitaker_describe(&result->forms[f].grammar, line, sizeof line);
      fold(tally.fingerprint, result->forms[f].orth);
      fold(tally.fingerprint, line);
    }
    if (result->form_count > tally.busiestCount) {
      tally.busiest = i;
      tally.busiestCount = result->form_count;
    }
  }
  tallies[k] = tally;
}

// NOTE: Combine in batch order so scheduling cannot change the fingerprint.
Tally combine(const std::vector<Tally>& tallies) {
  Tally total;
  for (const Tally& tally : tallies) {
    total.readings += tally.readings;
    total.fingerprint = (total.fingerprint ^ tally.fingerprint) * kFnvPrime;
    if (tally.busiestCount > total.busiestCount) {
      total.busiest = tally.busiest;
      total.busiestCount = tally.busiestCount;
    }
  }
  return total;
}

} // namespace

int main(int argc, char** argv) try {
  std::vector<std::string> words;
  for (int i = 1; i < argc; ++i) {
    std::ifstream file(argv[i], std::ios::binary);
    if (!file) {
      std::cerr << "cannot read " << argv[i] << '\n';
      return 1;
    }
    for (std::string& word : wordsOf({std::istreambuf_iterator<char>(file), {}}))
      words.push_back(std::move(word));
  }
  if (argc == 1)
    words = wordsOf("Gallia est omnis divisa in partes tres. "
                    "Arma virumque cano, Troiae qui primus ab oris. "
                    "Quo usque tandem abutere, Catilina, patientia nostra?");
  const std::size_t batches = (words.size() + kBatch - 1) / kBatch;

  using clock = std::chrono::steady_clock;
  const auto start = clock::now();
  std::vector<Tally> threaded(batches);
  std::atomic<std::size_t> next{0};
  const unsigned threads = std::max(1u, std::thread::hardware_concurrency());
  std::vector<std::future<void>> tasks;
  // NOTE: Workers claim batches dynamically to balance uneven lookup costs.
  for (unsigned t = 0; t < threads; ++t)
    tasks.push_back(std::async(std::launch::async, [&] {
      for (std::size_t k; (k = next++) < batches;)
        analyzeBatch(words, threaded, k);
    }));
  for (std::future<void>& task : tasks)
    task.get();
  const auto split = clock::now();

  std::vector<Tally> alone(batches);
  for (std::size_t k = 0; k < batches; ++k)
    analyzeBatch(words, alone, k);
  const auto done = clock::now();

  const Tally total = combine(threaded);
  const bool same = total.fingerprint == combine(alone).fingerprint;
  std::cout << words.size() << " words, " << total.readings << " readings, " << (same ? "the same" : "DIFFERENT")
            << " on one thread\n";

  // NOTE: Clone before reusing the source result.
  if (total.busiestCount > 0) {
    const Result scratch = newResult();
    const auto status = whitaker_analyze(words[total.busiest].c_str(), scratch.get());
    if (status == WHITAKER_OUT_OF_MEMORY)
      throw std::bad_alloc{};
    if (status == WHITAKER_OK) {
      const Result kept{whitaker_result_clone(scratch.get()), whitaker_result_free};
      if (!kept)
        throw std::bad_alloc{};
      if (whitaker_analyze("amo", scratch.get()) == WHITAKER_OUT_OF_MEMORY)
        throw std::bad_alloc{};
      std::cout << "most readings: " << words[total.busiest] << ", " << kept->form_count << ", the first "
                << kept->forms[0].orth << ' ' << kept->records[0].pos << '\n';
    }
  }

  using ms = std::chrono::duration<double, std::milli>;
  std::cerr << ms(split - start).count() << " ms on " << threads << " threads, " << ms(done - split).count()
            << " ms on one\n";
  return same ? 0 : 1;
} catch (const std::bad_alloc&) {
  std::cerr << "out of memory\n";
  return 1;
}
