#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

#include "facts.hpp"
#include "gen/expand/wl_stable/common/form.hpp"

namespace wl_stable::expand {

using ByLetter = std::array<std::vector<Form>, facts::kLetterCount>;

[[nodiscard]] inline std::array<bool, facts::kLetterCount>
wanted(std::string_view letters) noexcept {
  std::array<bool, facts::kLetterCount> want{};
  for (const char letter : letters)
    want[static_cast<std::size_t>(facts::letterIndex(letter))] = true;
  return want;
}

// INFO: Same spelling, and which comes first, compared over the folded letter,
// so `iam` and `jam` are one spelling.
[[nodiscard]] constexpr unsigned keyCharacter(char c) noexcept {
  const int index = facts::letterIndex(c);
  return index < 0 ? 0u : static_cast<unsigned>(index) + 1u;
}

[[nodiscard]] constexpr bool spellingLess(std::string_view a,
                                          std::string_view b) noexcept {
  const std::size_t n = a.size() < b.size() ? a.size() : b.size();
  for (std::size_t i = 0; i < n; ++i) {
    const unsigned ca = keyCharacter(a[i]), cb = keyCharacter(b[i]);
    if (ca != cb)
      return ca < cb;
  }
  return a.size() < b.size();
}

[[nodiscard]] constexpr bool spellingEqual(std::string_view a,
                                           std::string_view b) noexcept {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i)
    if (keyCharacter(a[i]) != keyCharacter(b[i]))
      return false;
  return true;
}

} // namespace wl_stable::expand
