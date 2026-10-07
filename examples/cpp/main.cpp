// Analyzes books on all the CPU's threads with std::async, then checks the answers against one thread. Books are plain
// text; anything but a letter separates words. With no books, three opening lines stand in for them.
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
#include <string>
#include <thread>
#include <vector>

#include <whitaker.h>

namespace {

constexpr std::size_t kBatch = 1024;

// FNV-1a, folding every reading's stem and grammar into one number.
constexpr std::uint64_t kFnvBasis = 1469598103934665603u;
constexpr std::uint64_t kFnvPrime = 1099511628211u;

struct Tally {
  long readings = 0;
  std::uint64_t fingerprint = kFnvBasis;
  std::size_t busiest = 0;
  int busiestCount = -1;
};

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

// Batch `k` of `words` into `tallies[k]`. Safe from any number of threads at once: each call has its own result and
// writes its own tally.
void analyzeBatch(const std::vector<std::string>& words, std::vector<Tally>& tallies, std::size_t k) {
  auto result = std::make_unique<WhitakerResult>();
  Tally tally;
  const std::size_t end = std::min(words.size(), (k + 1) * kBatch);
  for (std::size_t i = k * kBatch; i < end; ++i) {
    if (whitaker_analyze(words[i].c_str(), result.get()) != WHITAKER_OK)
      continue;
    tally.readings += result->count;
    for (int m = 0; m < result->count; ++m) {
      char line[64];
      whitaker_describe(&result->matches[m].grammar, line, sizeof line);
      fold(tally.fingerprint, result->matches[m].orth);
      fold(tally.fingerprint, line);
    }
    if (result->count > tally.busiestCount) {
      tally.busiest = i;
      tally.busiestCount = result->count;
    }
  }
  tallies[k] = tally;
}

// The batches' tallies, in batch order: the same however threads ran them.
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

int main(int argc, char** argv) {
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

  // Every thread takes the next batch until none is left, so a slow batch holds up no one.
  using clock = std::chrono::steady_clock;
  const auto start = clock::now();
  std::vector<Tally> threaded(batches);
  std::atomic<std::size_t> next{0};
  const unsigned threads = std::max(1u, std::thread::hardware_concurrency());
  std::vector<std::future<void>> tasks;
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

  // A result's strings can point into the result itself, so one that is kept is copied with whitaker_result_copy, never
  // `=`; scratch is then reused for "amo" and kept is unaffected.
  if (total.busiestCount > 0) {
    auto scratch = std::make_unique<WhitakerResult>();
    auto kept = std::make_unique<WhitakerResult>();
    if (whitaker_analyze(words[total.busiest].c_str(), scratch.get()) == WHITAKER_OK)
      whitaker_result_copy(kept.get(), scratch.get());
    (void)whitaker_analyze("amo", scratch.get());
    std::cout << "most readings: " << words[total.busiest] << ", " << kept->count << ", the first "
              << kept->matches[0].orth << ' ' << kept->matches[0].pos << '\n';
  }

  using ms = std::chrono::duration<double, std::milli>;
  std::cerr << ms(split - start).count() << " ms on " << threads << " threads, " << ms(done - split).count()
            << " ms on one\n";
  return same ? 0 : 1;
}
