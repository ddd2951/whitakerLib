#include "relationship_index.hpp"

#include "error/error.hpp"
#include "facts.hpp"
#include "src/search/relationship_schema.hpp"

#include <algorithm>
#include <bit>
#include <limits>
#include <unordered_map>
#include <utility>

namespace rel = whitaker::relationship;

namespace wl_stable::emitter::detail::relationship_index {
namespace {

constexpr std::uint16_t kNoDense = std::numeric_limits<std::uint16_t>::max();

[[nodiscard]] std::uint64_t mix(std::uint64_t hash,
                                std::uint64_t value) noexcept {
  hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
  return hash;
}

struct VectorHash {
  [[nodiscard]] std::size_t
  operator()(const std::vector<std::uint16_t>& values) const noexcept {
    std::uint64_t hash = 0x243f6a8885a308d3ULL;
    for (const std::uint16_t value : values)
      hash = mix(hash, value);
    return static_cast<std::size_t>(hash);
  }
};

} // namespace

DirectOutputMachine::DirectOutputMachine(
    const std::vector<ProgramWord>& words) {
  build(words);
}

std::size_t DirectOutputMachine::states() const noexcept {
  return m_states.size();
}

std::size_t DirectOutputMachine::transitions() const noexcept {
  return m_edges.size();
}

std::uint32_t DirectOutputMachine::root() const noexcept { return m_root; }

std::uint32_t DirectOutputMachine::rootOutput() const noexcept {
  return m_rootOutput;
}

const std::vector<std::uint64_t>&
DirectOutputMachine::packedStates() const noexcept {
  return m_states;
}

const std::vector<std::uint64_t>&
DirectOutputMachine::packedEdges() const noexcept {
  return m_edges;
}

const std::vector<std::uint8_t>&
DirectOutputMachine::resultCounts() const noexcept {
  return m_resultCounts;
}

bool DirectOutputMachine::matchesState(
    std::uint32_t id, std::uint8_t resultCount,
    const std::vector<Edge>& out) const noexcept {
  const State& state = m_nativeStates[id];
  if (state.resultCount != resultCount ||
      static_cast<std::size_t>(std::popcount(state.mask)) != out.size())
    return false;
  return std::equal(out.begin(), out.end(), m_nativeEdges.begin() + state.first,
                    [](const Edge& a, const Edge& b) {
                      return a.label == b.label && a.target == b.target &&
                             a.output == b.output;
                    });
}

void DirectOutputMachine::build(const std::vector<ProgramWord>& words) {
  std::vector<Node> trie(1);
  std::vector<std::uint32_t> path{0};
  std::string_view previous;
  for (const ProgramWord& word : words) {
    std::size_t common = 0;
    while (common < previous.size() && common < word.text.size() &&
           previous[common] == word.text[common])
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
    if (word.analysisCount == 0 ||
        word.analysisCount > rel::kMaximumResultsPerSpelling)
      error::fatal(
          "relationship emitter: terminal count exceeds image capacity");
    trie[path.back()].output = word.instruction;
    trie[path.back()].resultCount =
        static_cast<std::uint8_t>(word.analysisCount);
    previous = word.text;
  }

  std::unordered_multimap<std::uint64_t, std::uint32_t> registry;
  registry.reserve(trie.size() / 2);
  std::vector<Edge> out;
  out.reserve(facts::kLetterCount);
  for (std::size_t n = trie.size(); n-- > 0;) {
    const bool terminal = trie[n].resultCount != 0;
    std::uint32_t base = terminal ? trie[n].output : kNone;
    if (!terminal && trie[n].firstChild != kNone)
      base = trie[trie[n].firstChild].baseOutput;
    if (base == kNone)
      error::fatal("relationship emitter: the trie contains a dead state");
    trie[n].baseOutput = base;

    out.clear();
    for (std::uint32_t child = trie[n].firstChild; child != kNone;
         child = trie[child].nextSibling) {
      if (trie[child].baseOutput < base)
        error::fatal(
            "relationship emitter: output addresses are not monotonic");
      out.push_back(
          Edge{.target = trie[child].canonical,
               .output = trie[child].baseOutput - base,
               .label = static_cast<std::uint8_t>(trie[child].label - 'a')});
    }

    std::uint64_t hash = terminal ? 0x51f15e5dULL : 0x9d2c5680ULL;
    hash = mix(hash, trie[n].resultCount);
    for (const Edge& edge : out) {
      hash = mix(hash, edge.label);
      hash = mix(hash, edge.target);
      hash = mix(hash, edge.output);
    }
    std::uint32_t canonical = kNone;
    const auto [begin, end] = registry.equal_range(hash);
    for (auto candidate = begin; candidate != end; ++candidate)
      if (matchesState(candidate->second, trie[n].resultCount, out)) {
        canonical = candidate->second;
        break;
      }
    if (canonical == kNone) {
      canonical = static_cast<std::uint32_t>(m_nativeStates.size());
      std::uint32_t mask = 0;
      for (const Edge& edge : out)
        mask |= 1u << edge.label;
      m_nativeStates.push_back(
          State{.first = static_cast<std::uint32_t>(m_nativeEdges.size()),
                .mask = mask,
                .resultCount = trie[n].resultCount});
      m_nativeEdges.insert(m_nativeEdges.end(), out.begin(), out.end());
      registry.emplace(hash, canonical);
    }
    trie[n].canonical = canonical;
  }
  m_root = trie.front().canonical;
  m_rootOutput = trie.front().baseOutput;

  m_states.reserve(m_nativeStates.size());
  m_resultCounts.reserve(m_nativeStates.size());
  for (const State& state : m_nativeStates) {
    m_states.push_back(state.first |
                       (static_cast<std::uint64_t>(state.mask) << 32));
    m_resultCounts.push_back(state.resultCount);
  }
  m_edges.reserve(m_nativeEdges.size());
  for (const Edge& edge : m_nativeEdges)
    m_edges.push_back(edge.target |
                      (static_cast<std::uint64_t>(edge.output) << 32));
}

Program build(const std::vector<Word>& words,
              const std::vector<Analysis>& analyses) {
  constexpr std::size_t sparseCapacity = 1u << 18;
  const auto sparseOf = [](const Analysis& analysis) {
    return analysis.dictionary | (std::uint32_t{analysis.stem} << 16);
  };
  std::vector<std::vector<std::uint16_t>> sparseTargets(sparseCapacity);
  for (const Analysis& analysis : analyses)
    sparseTargets[sparseOf(analysis)].push_back(analysis.description);

  Program data;
  std::vector<std::uint16_t> sparseToDense(sparseCapacity, kNoDense);
  std::unordered_map<std::vector<std::uint16_t>, std::uint16_t, VectorHash>
      paradigmIds;
  paradigmIds.reserve(512);
  for (std::uint32_t sparse = 0; sparse < sparseTargets.size(); ++sparse) {
    auto& relations = sparseTargets[sparse];
    if (relations.empty())
      continue;
    std::ranges::sort(relations);
    relations.erase(std::unique(relations.begin(), relations.end()),
                    relations.end());
    if (relations.size() > rel::kMaximumTargetsPerParadigm ||
        data.lexemes.size() >= kNoDense)
      error::fatal("relationship emitter: native ID capacity exceeded");
    const auto candidate = static_cast<std::uint16_t>(data.paradigms.size());
    auto [paradigm, inserted] = paradigmIds.emplace(relations, candidate);
    if (inserted) {
      data.paradigms.push_back(
          Paradigm{static_cast<std::uint32_t>(data.targets.size()),
                   static_cast<std::uint8_t>(relations.size())});
      data.targets.insert(data.targets.end(), relations.begin(),
                          relations.end());
    }
    const std::uint16_t dense = static_cast<std::uint16_t>(data.lexemes.size());
    sparseToDense[sparse] = dense;
    data.lexemes.push_back(Lexeme{
        static_cast<std::uint16_t>(sparse),
        static_cast<std::uint8_t>((sparse >> 16) & 3u), paradigm->second});
  }

  data.words.reserve(words.size());
  data.instructions.reserve(analyses.size() * 2);
  for (const Word& word : words) {
    if (data.instructions.size() > std::numeric_limits<std::uint32_t>::max())
      error::fatal(
          "relationship emitter: instruction address exceeds uint32_t");
    data.words.push_back(ProgramWord{
        word.text, static_cast<std::uint32_t>(data.instructions.size()),
        word.analysisCount});
    std::uint16_t previous = kNoDense;
    for (std::uint32_t row = word.analysisFirst;
         row < word.analysisFirst + word.analysisCount; ++row) {
      const Analysis& analysis = analyses[row];
      const std::uint16_t dense = sparseToDense[sparseOf(analysis)];
      if (dense == kNoDense)
        error::fatal("relationship emitter: dense lexeme is missing");
      const Lexeme& lexeme = data.lexemes[dense];
      const Paradigm& paradigm = data.paradigms[lexeme.paradigm];
      const std::uint16_t description = analysis.description;
      const auto begin = data.targets.begin() + paradigm.firstTarget;
      const auto end = begin + paradigm.count;
      const auto found = std::lower_bound(begin, end, description);
      if (found == end || *found != description)
        error::fatal("relationship emitter: target is absent from paradigm");
      const std::uint8_t local = static_cast<std::uint8_t>(found - begin);
      const bool change = dense != previous;
      data.instructions.push_back(
          static_cast<std::byte>(local | (change ? rel::kLexemeChange : 0u)));
      if (change) {
        data.instructions.push_back(static_cast<std::byte>(dense));
        data.instructions.push_back(static_cast<std::byte>(dense >> 8));
        previous = dense;
      }
    }
  }
  return data;
}

} // namespace wl_stable::emitter::detail::relationship_index
