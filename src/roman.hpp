#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace whitaker::roman {

// U is intentionally excluded even though lookup otherwise folds v onto u.
inline constexpr std::string_view kLetters{"MDCLXVI"};

// INFO: Roman_Number as its minimal automaton. A state is named by the shortest numeral that reaches it.
// clang-format off
enum State : std::uint8_t {
  reject, start,
  M, MM, MMM, MMMM,
  D, DC, DCC, C, CC, CCC, CM,
  L, LX, X, XX, XXX, XC,
  V, VI, I, II, III, IX
};
// clang-format on

struct Step {
  State to;
  std::uint16_t add;
};

inline constexpr auto kColumns = [] {
  std::array<std::uint8_t, 256> column;
  column.fill(kLetters.size());
  for (std::uint8_t i = 0; i < kLetters.size(); ++i) {
    column[static_cast<unsigned char>(kLetters[i])] = i;
    column[static_cast<unsigned char>(kLetters[i] - 'A' + 'a')] = i;
  }
  return column;
}();

// INFO: A row per state, a column per letter and an empty one for any other character; {} rejects.
inline constexpr auto kSteps = [] {
  std::array<std::array<Step, kLetters.size() + 1>, IX + 1> s{};

  // INFO: Any state may go on to a lower digit, opened by its letter's value.
  constexpr std::array<State, kLetters.size()> opens{M, D, C, L, X, V, I};
  constexpr std::array<std::uint16_t, kLetters.size()> values{1000, 500, 100, 50, 10, 5, 1};
  const auto digit = [](unsigned state) { return state >= V ? 4 : state >= L ? 3 : state >= D ? 2 : state >= M; };
  for (unsigned from = start; from <= IX; ++from)
    for (std::size_t k = 0; k < kLetters.size(); ++k)
      if (digit(opens[k]) > digit(from))
        s[from][k] = {opens[k], values[k]};

  const auto go = [&s](State from, char letter, State to, std::uint16_t add) {
    s[from][kLetters.find(letter)] = {to, add};
  };
  go(M, 'M', MM, 1000);
  go(MM, 'M', MMM, 1000);
  go(MMM, 'M', MMMM, 1000);

  go(D, 'C', DC, 100);
  go(DC, 'C', DCC, 100);
  go(DCC, 'C', CCC, 100);
  go(C, 'M', CM, 800);
  go(C, 'D', D, 300);
  go(C, 'C', CC, 100);
  go(CC, 'D', CM, 100);
  go(CC, 'C', CCC, 100);
  go(CCC, 'C', CM, 100);

  go(L, 'X', LX, 10);
  go(LX, 'X', XX, 10);
  go(X, 'C', XC, 80);
  go(X, 'L', XC, 30);
  go(X, 'X', XX, 10);
  go(XX, 'X', XXX, 10);
  go(XXX, 'X', XC, 10);

  go(V, 'I', VI, 1);
  go(VI, 'I', II, 1);
  go(I, 'X', IX, 8);
  go(I, 'V', IX, 3);
  go(I, 'I', II, 1);
  go(II, 'I', III, 1);
  go(III, 'I', IX, 1);
  return s;
}();

} // namespace whitaker::roman

namespace whitaker {

// Returns 0 for non-numerals. The accepted historical forms intentionally include IIII and VIIII for compatibility with
// Whitaker's Words.
// TODO: Bad_Roman_Number, WORDS' fallback that reads ill-formed numerals (IIX = 8), is not ported.
[[nodiscard]] constexpr unsigned romanValue(std::string_view word) noexcept {
  roman::State state = roman::start;
  unsigned total = 0;
  for (const char c : word) {
    const roman::Step step = roman::kSteps[state][roman::kColumns[static_cast<unsigned char>(c)]];
    if (step.to == roman::reject)
      return 0;
    total += step.add;
    state = step.to;
  }
  return total;
}

} // namespace whitaker
