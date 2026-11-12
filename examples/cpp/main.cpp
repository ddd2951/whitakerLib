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

#include <latin_generated.hpp>
#include <whitaker.hpp>

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

void fold(std::uint64_t& hash, const char* text) {
  for (; *text != '\0'; ++text)
    hash = (hash ^ static_cast<unsigned char>(*text)) * kFnvPrime;
  hash = (hash ^ 0xff) * kFnvPrime;
}

std::vector<std::string> wordsOf(const std::string& text) {
  std::vector<std::string> words{};
  std::string word{};
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
void analyzeBatch(const std::vector<std::string>& words, std::vector<Tally>& tallies, const std::size_t k) {
  whitaker::Result result{};
  Tally tally{};
  const std::size_t end = std::min(words.size(), (k + 1) * kBatch);
  for (std::size_t i = k * kBatch; i < end; ++i) {
    const whitaker::Status status = whitaker::analyze(result, words[i]);
    if (status == whitaker::Status::outOfMemory)
      throw std::bad_alloc{};
    if (status != whitaker::Status::ok)
      continue;
    const auto forms = whitaker::readings(result);
    tally.readings += static_cast<long>(forms.size());
    for (const auto& reading : forms) {
      const whitaker::ReadingDetail detail = whitaker::decode(result, whitaker::WordId{0}, reading);
      const auto& form = detail.form;
      char line[64];
      latin::describe(form.grammar, line, sizeof line);
      fold(tally.fingerprint, form.stemSpelling);
      fold(tally.fingerprint, line);
    }
    if (forms.size() > tally.busiestCount) {
      tally.busiest = i;
      tally.busiestCount = forms.size();
    }
  }
  tallies[k] = tally;
}

// NOTE: Combine in batch order so scheduling cannot change the fingerprint.
Tally combine(const std::vector<Tally>& tallies) {
  Tally total{};
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

int main(const int argc, char** const argv) try {
  std::vector<std::string> words{};
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

  using Clock = std::chrono::steady_clock;
  const auto start = Clock::now();
  std::vector<Tally> threaded(batches);
  std::atomic<std::size_t> next{0};
  const unsigned threads = std::max(1u, std::thread::hardware_concurrency());
  std::vector<std::future<void>> tasks{};
  // NOTE: Workers claim batches dynamically to balance uneven lookup costs.
  for (unsigned t = 0; t < threads; ++t)
    tasks.push_back(std::async(std::launch::async, [&] {
      for (std::size_t k; (k = next++) < batches;)
        analyzeBatch(words, threaded, k);
    }));
  for (std::future<void>& task : tasks)
    task.get();
  const auto split = Clock::now();

  std::vector<Tally> alone(batches);
  for (std::size_t k = 0; k < batches; ++k)
    analyzeBatch(words, alone, k);
  const auto done = Clock::now();

  const Tally total = combine(threaded);
  const bool same = total.fingerprint == combine(alone).fingerprint;
  std::cout << words.size() << " words, " << total.readings << " readings, " << (same ? "the same" : "DIFFERENT")
            << " on one thread\n";

  // NOTE: Clone before reusing the source result.
  if (total.busiestCount > 0) {
    whitaker::Result scratch{};
    const whitaker::Status status = whitaker::analyze(scratch, words[total.busiest]);
    if (status == whitaker::Status::outOfMemory)
      throw std::bad_alloc{};
    if (status == whitaker::Status::ok) {
      const std::optional kept = whitaker::clone(scratch);
      if (!kept)
        throw std::bad_alloc{};
      if (whitaker::analyze(scratch, "amo") == whitaker::Status::outOfMemory)
        throw std::bad_alloc{};
      const whitaker::ReadingDetail detail = whitaker::detail(*kept, whitaker::WordId{0}, 0);
      std::cout << "most readings: " << words[total.busiest] << ", " << whitaker::readings(*kept).size()
                << ", the first " << detail.form.stemSpelling << ' ' << detail.partName << '\n';
    }
  }

  using Milliseconds = std::chrono::duration<double, std::milli>;
  std::cerr << Milliseconds(split - start).count() << " ms on " << threads << " threads, "
            << Milliseconds(done - split).count() << " ms on one\n";
  return same ? 0 : 1;
} catch (const std::bad_alloc&) {
  std::cerr << "out of memory\n";
  return 1;
}
