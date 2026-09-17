#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace wl_stable::emitter::detail::relationship_index {

struct Word {
  std::string text;
  std::uint32_t analysisFirst{};
  std::uint16_t analysisCount{};
};

struct Analysis {
  std::uint16_t dictionary{};
  std::uint8_t stem{};
  std::uint16_t description{};
};

struct ProgramWord {
  std::string text;
  std::uint32_t instruction{};
  std::uint16_t analysisCount{};
};

struct Lexeme {
  std::uint16_t dictionary{};
  std::uint8_t stem{};
  std::uint16_t paradigm{};
};

struct Paradigm {
  std::uint32_t firstTarget{};
  std::uint8_t count{};
};

struct Program {
  std::vector<Lexeme> lexemes;
  std::vector<Paradigm> paradigms;
  std::vector<std::uint16_t> targets;
  std::vector<std::byte> instructions;
  std::vector<ProgramWord> words;
};

class DirectOutputMachine {
public:
  explicit DirectOutputMachine(const std::vector<ProgramWord>& words);

  [[nodiscard]] std::size_t states() const noexcept;
  [[nodiscard]] std::size_t transitions() const noexcept;
  [[nodiscard]] std::uint32_t root() const noexcept;
  [[nodiscard]] std::uint32_t rootOutput() const noexcept;
  [[nodiscard]] const std::vector<std::uint64_t>& packedStates() const noexcept;
  [[nodiscard]] const std::vector<std::uint64_t>& packedEdges() const noexcept;
  [[nodiscard]] const std::vector<std::uint8_t>& resultCounts() const noexcept;

private:
  static constexpr std::uint32_t kNone =
      std::numeric_limits<std::uint32_t>::max();

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

  struct Edge {
    std::uint32_t target{};
    std::uint32_t output{};
    std::uint8_t label{};
  };

  struct State {
    std::uint32_t first{};
    std::uint32_t mask{};
    std::uint8_t resultCount{};
  };

  [[nodiscard]] bool matchesState(std::uint32_t id, std::uint8_t resultCount,
                                  const std::vector<Edge>& out) const noexcept;
  void build(const std::vector<ProgramWord>& words);

  std::vector<State> m_nativeStates;
  std::vector<Edge> m_nativeEdges;
  std::vector<std::uint64_t> m_states;
  std::vector<std::uint64_t> m_edges;
  std::vector<std::uint8_t> m_resultCounts;
  std::uint32_t m_root{};
  std::uint32_t m_rootOutput{};
};

[[nodiscard]] Program build(const std::vector<Word>& words,
                            const std::vector<Analysis>& analyses);

} // namespace wl_stable::emitter::detail::relationship_index
