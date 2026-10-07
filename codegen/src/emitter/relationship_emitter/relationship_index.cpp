#include "relationship_index.hpp"

#include "error/error.hpp"
#include "facts.hpp"
#include "src/search/relationship_schema.hpp"

#include <algorithm>
#include <format>
#include <limits>
#include <map>
#include <span>
#include <unordered_map>
#include <utility>

namespace rel = whitaker::relationship;

namespace emitter {
namespace {

constexpr std::uint16_t kNoDense = std::numeric_limits<std::uint16_t>::max();

[[nodiscard]] std::uint64_t mix(std::uint64_t hash, std::uint64_t value) noexcept {
  hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
  return hash;
}

} // namespace

Machine buildMachine(const std::vector<ProgramWord>& words) {
  constexpr std::uint32_t kNone = std::numeric_limits<std::uint32_t>::max();
  struct Node {
    std::uint32_t firstChild{kNone};
    std::uint32_t lastChild{kNone};
    std::uint32_t nextSibling{kNone};
    std::uint32_t canonical{kNone};
    std::uint32_t output{kNone};
    std::uint32_t baseOutput{};
    std::uint8_t resultCount{};
    char label{};
  };

  std::vector<Node> trie(1);
  std::vector<std::uint32_t> path{0};
  std::string_view previous;
  for (const ProgramWord& word : words) {
    std::size_t common = 0;
    while (common < previous.size() && common < word.text.size() && previous[common] == word.text[common])
      ++common;
    path.resize(common + 1);
    for (std::size_t i = common; i < word.text.size(); ++i) {
      const std::uint32_t parent = path.back();
      const std::uint32_t child = static_cast<std::uint32_t>(trie.size());
      trie.push_back(Node{.label = word.text[i]});
      if (trie[parent].firstChild == kNone)
        trie[parent].firstChild = child;
      else
        trie[trie[parent].lastChild].nextSibling = child;
      trie[parent].lastChild = child;
      path.push_back(child);
    }
    trie[path.back()].output = word.instruction;
    trie[path.back()].resultCount = static_cast<std::uint8_t>(word.analysisCount);
    previous = word.text;
  }

  // NOTE: Two states are one when their result count, letters and transitions match; the transitions are in letter
  //  order.
  Machine machine;
  const auto matches = [&machine](const rel::StateRecord& candidate, const rel::StateRecord& state,
                                  std::span<const rel::TransitionRecord> out) {
    return candidate.resultCount == state.resultCount && candidate.letterMask == state.letterMask &&
           std::ranges::equal(out, std::span{machine.transitions}.subspan(candidate.firstTransition, out.size()),
                              [](const rel::TransitionRecord& a, const rel::TransitionRecord& b) {
                                return a.target == b.target && a.output == b.output;
                              });
  };
  std::unordered_multimap<std::uint64_t, std::uint32_t> registry;
  registry.reserve(trie.size() / 2);
  std::vector<rel::TransitionRecord> out;
  out.reserve(facts::kLetterCount);
  for (std::size_t n = trie.size(); n-- > 0;) {
    const std::uint32_t base = trie[n].resultCount != 0 ? trie[n].output : trie[trie[n].firstChild].baseOutput;
    trie[n].baseOutput = base;

    rel::StateRecord state{.resultCount = trie[n].resultCount};
    out.clear();
    for (std::uint32_t child = trie[n].firstChild; child != kNone; child = trie[child].nextSibling) {
      state.letterMask |= 1u << (trie[child].label - 'a');
      out.push_back({.target = trie[child].canonical, .output = trie[child].baseOutput - base});
    }

    std::uint64_t hash = mix(0x9d2c5680ULL, state.resultCount);
    hash = mix(hash, state.letterMask);
    for (const rel::TransitionRecord& edge : out) {
      hash = mix(hash, edge.target);
      hash = mix(hash, edge.output);
    }
    std::uint32_t canonical = kNone;
    const auto [begin, end] = registry.equal_range(hash);
    for (auto candidate = begin; candidate != end; ++candidate)
      if (matches(machine.states[candidate->second], state, out)) {
        canonical = candidate->second;
        break;
      }
    if (canonical == kNone) {
      canonical = static_cast<std::uint32_t>(machine.states.size());
      state.firstTransition = static_cast<std::uint32_t>(machine.transitions.size());
      machine.states.push_back(state);
      machine.transitions.insert(machine.transitions.end(), out.begin(), out.end());
      registry.emplace(hash, canonical);
    }
    trie[n].canonical = canonical;
  }
  machine.root = trie.front().canonical;
  machine.rootOutput = trie.front().baseOutput;
  return machine;
}

Program build(const std::vector<Word>& words, const std::vector<Analysis>& analyses) {
  constexpr unsigned kStemShift = 16;
  constexpr std::size_t sparseCapacity = std::size_t{facts::kStemColumnCount} << kStemShift;
  const auto sparseOf = [](const Analysis& analysis) {
    return analysis.dictionary | (std::uint32_t{analysis.stem} << kStemShift);
  };
  std::vector<std::vector<std::uint16_t>> sparseTargets(sparseCapacity);
  for (const Analysis& analysis : analyses)
    sparseTargets[sparseOf(analysis)].push_back(analysis.description);

  Program data;
  std::vector<std::uint16_t> sparseToDense(sparseCapacity, kNoDense);
  std::map<std::vector<std::uint16_t>, std::uint16_t> paradigmIds;
  for (std::uint32_t sparse = 0; sparse < sparseTargets.size(); ++sparse) {
    auto& relations = sparseTargets[sparse];
    if (relations.empty())
      continue;
    std::ranges::sort(relations);
    relations.erase(std::unique(relations.begin(), relations.end()), relations.end());
    if (relations.size() > rel::kMaximumTargetsPerParadigm)
      error::fatal(std::format("image: a paradigm with more than {} targets", rel::kMaximumTargetsPerParadigm));
    if (data.lexemes.size() >= kNoDense)
      error::fatal("image: more lexemes than 16 bits can name");
    const auto candidate = static_cast<std::uint16_t>(data.paradigms.size());
    auto [paradigm, inserted] = paradigmIds.emplace(relations, candidate);
    if (inserted) {
      data.paradigms.push_back(rel::ParadigmRecord{.firstTarget = static_cast<std::uint32_t>(data.targets.size()),
                                                   .count = static_cast<std::uint8_t>(relations.size())});
      data.targets.insert(data.targets.end(), relations.begin(), relations.end());
    }
    const std::uint16_t dense = static_cast<std::uint16_t>(data.lexemes.size());
    sparseToDense[sparse] = dense;
    data.lexemes.push_back(rel::LexemeRecord{.dictionary = static_cast<std::uint16_t>(sparse),
                                             .stem = static_cast<std::uint8_t>(sparse >> kStemShift),
                                             .paradigm = paradigm->second});
  }

  data.words.reserve(words.size());
  data.instructions.reserve(analyses.size() * 2);
  for (const Word& word : words) {
    if (data.instructions.size() > std::numeric_limits<std::uint32_t>::max())
      error::fatal("image: instruction addresses past 32 bits");
    data.words.push_back(
        ProgramWord{word.text, static_cast<std::uint32_t>(data.instructions.size()), word.analysisCount});
    std::uint16_t previous = kNoDense;
    for (std::uint32_t row = word.analysisFirst; row < word.analysisFirst + word.analysisCount; ++row) {
      const Analysis& analysis = analyses[row];
      const std::uint16_t dense = sparseToDense[sparseOf(analysis)];
      const rel::LexemeRecord& lexeme = data.lexemes[dense];
      const rel::ParadigmRecord& paradigm = data.paradigms[lexeme.paradigm];
      const std::uint16_t description = analysis.description;
      const auto begin = data.targets.begin() + paradigm.firstTarget;
      const auto end = begin + paradigm.count;
      const auto found = std::lower_bound(begin, end, description);
      const std::uint8_t local = static_cast<std::uint8_t>(found - begin);
      const bool change = dense != previous;
      data.instructions.push_back(static_cast<std::byte>(local | (change ? rel::kLexemeChange : 0u)));
      if (change) {
        data.instructions.push_back(static_cast<std::byte>(dense));
        data.instructions.push_back(static_cast<std::byte>(dense >> 8));
        previous = dense;
      }
    }
  }
  return data;
}

} // namespace emitter
