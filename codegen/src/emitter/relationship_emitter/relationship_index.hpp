#pragma once

#include "src/search/relationship_schema.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace emitter {

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

struct Program {
  std::vector<whitaker::relationship::LexemeRecord> lexemes;
  std::vector<whitaker::relationship::ParadigmRecord> paradigms;
  std::vector<std::uint16_t> targets;
  std::vector<std::byte> instructions;
  std::vector<ProgramWord> words;
};

// The spelling automaton as stored, and where its walk starts.
struct Machine {
  std::vector<whitaker::relationship::StateRecord> states;
  std::vector<whitaker::relationship::TransitionRecord> transitions;
  std::uint32_t root{};
  std::uint32_t rootOutput{};
};

[[nodiscard]] Machine buildMachine(const std::vector<ProgramWord>& words);

[[nodiscard]] Program build(const std::vector<Word>& words, const std::vector<Analysis>& analyses);

} // namespace emitter
