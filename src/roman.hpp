#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace whitaker::roman {

// NOTE: U is intentionally excluded even though lookup otherwise folds v onto u.
inline constexpr std::string_view kLetters{"MDCLXVI"};

// INFO: Roman_Number as its minimal automaton. A state is named by the shortest numeral that reaches it.
// clang-format off
enum State : std::uint8_t {
  reject,
  start,
  m,
  mm,
  mmm,
  mmmm,
  d,
  dc,
  dcc,
  c,
  cc,
  ccc,
  cm,
  l,
  lx,
  x,
  xx,
  xxx,
  xc,
  v,
  vi,
  i,
  ii,
  iii,
  ix,
};
// clang-format on

struct Step {
  State to{};
  std::uint16_t add{};
};
static_assert(std::is_trivially_copyable_v<Step>);

inline constexpr auto kColumns = [] {
  std::array<std::uint8_t, 256> column{};
  column.fill(kLetters.size());
  for (std::uint8_t letterIndex = 0; letterIndex < kLetters.size(); ++letterIndex) {
    column[static_cast<unsigned char>(kLetters[letterIndex])] = letterIndex;
    column[static_cast<unsigned char>(kLetters[letterIndex] - 'A' + 'a')] = letterIndex;
  }
  return column;
}();

// INFO: A row per state, a column per letter and an empty one for any other character; {} rejects.
inline constexpr auto kSteps = [] {
  std::array<std::array<Step, kLetters.size() + 1>, ix + 1> s{};

  // INFO: Any state may go on to a lower digit, opened by its letter's value.
  constexpr std::array<State, kLetters.size()> opens{m, d, c, l, x, v, i};
  constexpr std::array<std::uint16_t, kLetters.size()> values{1000, 500, 100, 50, 10, 5, 1};
  const auto digit = [](const unsigned state) { return state >= v ? 4 : state >= l ? 3 : state >= d ? 2 : state >= m; };
  for (unsigned from = start; from <= ix; ++from)
    for (std::size_t k = 0; k < kLetters.size(); ++k)
      if (digit(opens[k]) > digit(from))
        s[from][k] = {opens[k], values[k]};

  const auto go = [&s](const State from, const char letter, const State to, const std::uint16_t add) {
    s[from][kLetters.find(letter)] = {to, add};
  };
  go(m, 'M', mm, 1000);
  go(mm, 'M', mmm, 1000);
  go(mmm, 'M', mmmm, 1000);

  go(d, 'C', dc, 100);
  go(dc, 'C', dcc, 100);
  go(dcc, 'C', ccc, 100);
  go(c, 'M', cm, 800);
  go(c, 'D', d, 300);
  go(c, 'C', cc, 100);
  go(cc, 'D', cm, 100);
  go(cc, 'C', ccc, 100);
  go(ccc, 'C', cm, 100);

  go(l, 'X', lx, 10);
  go(lx, 'X', xx, 10);
  go(x, 'C', xc, 80);
  go(x, 'L', xc, 30);
  go(x, 'X', xx, 10);
  go(xx, 'X', xxx, 10);
  go(xxx, 'X', xc, 10);

  go(v, 'I', vi, 1);
  go(vi, 'I', ii, 1);
  go(i, 'X', ix, 8);
  go(i, 'V', ix, 3);
  go(i, 'I', ii, 1);
  go(ii, 'I', iii, 1);
  go(iii, 'I', ix, 1);
  return s;
}();

} // namespace whitaker::roman

namespace whitaker {

// INFO: Returns 0 for non-numerals.
// NOTE: The accepted historical forms include IIII and VIIII on purpose, as Whitaker's Words does.
// TODO: Bad_Roman_Number, WORDS' fallback that reads ill-formed numerals (IIX = 8), is not ported.
[[nodiscard]] constexpr unsigned romanValue(const std::string_view word) {
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
