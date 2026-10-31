#pragma once

#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace tester {

enum class LineKind {
  Inflection,
  DictionaryForm,
  // INFO: Meaning: a trailing CR comes from DICTLINE.GEN.
  Meaning,
  RouteMarker,
  Explanation,
  Addon,
  Unknown,
  Trimmed,
  Blank,
};

LineKind classify(std::string_view line);

inline std::string folded(std::string_view text) {
  std::string out;
  for (unsigned char c : text) {
    const char lower = static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    out += lower == 'j' ? 'i' : lower == 'v' ? 'u' : lower;
  }
  return out;
}

inline std::vector<std::string_view> wordsOf(std::string_view line) {
  std::vector<std::string_view> out;
  for (auto word : line | std::views::split(' '))
    if (!word.empty())
      out.emplace_back(word.begin(), word.end());
  return out;
}

} // namespace tester
