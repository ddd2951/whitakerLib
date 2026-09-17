#include "roman.hpp"

namespace {

[[nodiscard]] char upper(char c) noexcept {
  return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
}

// U is intentionally excluded even though lookup otherwise folds v onto u.
[[nodiscard]] bool isRomanDigit(char c) noexcept {
  switch (upper(c)) {
  case 'M':
  case 'D':
  case 'C':
  case 'L':
  case 'X':
  case 'V':
  case 'I':
    return true;
  default:
    return false;
  }
}

} // namespace

namespace whitaker {

// INFO: Roman_Number, words_engine-roman_numerals_package.adb:68,
//  transliterated.
unsigned romanValue(std::string_view word) noexcept {
  if (word.empty())
    return 0;
  for (const char c : word)
    if (!isRomanDigit(c))
      return 0;

  unsigned total = 0;
  int j = static_cast<int>(word.size()) - 1;
  const auto at = [&](int i) {
    return upper(word[static_cast<std::size_t>(i)]);
  };

  while (j >= 0) {
    if (at(j) == 'I') {
      total += 1;
      if (--j < 0)
        return total;
      while (at(j) == 'I') {
        total += 1;
        if (total >= 5)
          return 0;
        if (--j < 0)
          return total;
      }
    }

    if (at(j) == 'V') {
      total += 5;
      if (--j < 0)
        return total;
      if (at(j) == 'I' && total == 5) {
        total -= 1;
        if (--j < 0)
          return total;
      }
      if (at(j) == 'I' || at(j) == 'V')
        return 0;
    }

    if (at(j) == 'X') {
      total += 10;
      if (--j < 0)
        return total;
      while (at(j) == 'X') {
        total += 10;
        if (total >= 50)
          return 0;
        if (--j < 0)
          return total;
      }
      if (at(j) == 'I' && total == 10) {
        total -= 1;
        if (--j < 0)
          return total;
      }
      if (at(j) == 'I' || at(j) == 'V')
        return 0;
    }

    if (at(j) == 'L') {
      total += 50;
      if (--j < 0)
        return total;
      if (at(j) == 'X' && total <= 59) {
        total -= 10;
        if (--j < 0)
          return total;
      }
      if (at(j) == 'I' || at(j) == 'V' || at(j) == 'X' || at(j) == 'L')
        return 0;
      // NOTE: The source's own C-after-L arm, kept as it is.
      if (at(j) == 'C') {
        total += 100;
        if (--j < 0)
          return total;
        if (at(j) == 'X' && total == 100) {
          total -= 10;
          if (--j < 0)
            return total;
        }
      }
      if (at(j) == 'I' || at(j) == 'V' || at(j) == 'X' || at(j) == 'L')
        return 0;
    }

    if (at(j) == 'C') {
      total += 100;
      if (--j < 0)
        return total;
      while (at(j) == 'C') {
        total += 100;
        if (total >= 500)
          return 0;
        if (--j < 0)
          return total;
      }
      if (at(j) == 'X' && total <= 109) {
        total -= 10;
        if (--j < 0)
          return total;
      }
      if (at(j) == 'I' || at(j) == 'V' || at(j) == 'X' || at(j) == 'L')
        return 0;
    }

    if (at(j) == 'D') {
      total += 500;
      if (--j < 0)
        return total;
      if (at(j) == 'C' && total <= 599) {
        total -= 100;
        if (--j < 0)
          return total;
      }
      if (at(j) == 'M') {
        total += 1000;
        if (--j < 0)
          return total;
      }
      if (at(j) == 'C' && total <= 1099) {
        total -= 100;
        if (--j < 0)
          return total;
      }
      if (at(j) == 'I' || at(j) == 'V' || at(j) == 'X' || at(j) == 'L' ||
          at(j) == 'C' || at(j) == 'D')
        return 0;
    }

    if (at(j) == 'M') {
      total += 1000;
      if (--j < 0)
        return total;
      while (at(j) == 'M') {
        total += 1000;
        if (total >= 5000)
          return 0;
        if (--j < 0)
          return total;
      }
      if (at(j) == 'C' && total <= 1099) {
        total -= 100;
        if (--j < 0)
          return total;
      }
      if (at(j) == 'I' || at(j) == 'V' || at(j) == 'X' || at(j) == 'L' ||
          at(j) == 'C' || at(j) == 'D')
        return 0;
    }
  }

  return total;
}

} // namespace whitaker
