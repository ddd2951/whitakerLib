#include "ada_parse.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <ranges>

namespace tester {
namespace {

using namespace std::string_view_literals;

// INFO: Line starts, from the literals in words_engine-tricks.adb: every pseudo record a trick adds, and every text it
//  stores in Xp.Xxx/Yyy_Meaning.
constexpr std::array kMarkers{"Syncope"sv, "Word mod"sv, "Slur "sv, "Two words"sv, "Bad Roman Numeral"sv};
constexpr std::array kExplanations{"Syncopated perfect"sv,
                                   "An initial '"sv,
                                   "An internal '"sv,
                                   "A Terminal '"sv,
                                   "A doubled consonant"sv,
                                   "May be 2 words combined"sv,
                                   "It is very likely a compound number"sv,
                                   "Some forms of eo stem"sv};
constexpr std::array kParts{"N"sv,   "V"sv,    "ADJ"sv,  "ADV"sv,    "PRON"sv, "VPAR"sv, "SUPINE"sv,
                            "NUM"sv, "PREP"sv, "CONJ"sv, "INTERJ"sv, "PACK"sv, "X"sv};
constexpr std::array kAddons{"TACKON"sv, "PREFIX"sv, "SUFFIX"sv};
bool among(std::string_view word, const auto& list) { return std::ranges::contains(list, word); }

std::string_view trimRight(std::string_view line) {
  while (!line.empty() && (line.back() == ' ' || line.back() == '\r'))
    line.remove_suffix(1);
  return line;
}

bool hasCodes(std::string_view line) {
  for (auto at = line.find('['); at != std::string_view::npos; at = line.find('[', at + 1))
    if (at + 6 < line.size() && line[at + 6] == ']' &&
        std::ranges::all_of(line.substr(at + 1, 5), [](char c) { return std::isupper(c); }))
      return true;
  return false;
}

std::optional<std::string_view> leadingPart(std::string_view line) {
  if (line.empty() || line.front() == ' ')
    return std::nullopt;
  const auto gap = line.find(' ');
  if (gap == std::string_view::npos || !line.substr(gap).starts_with("  "))
    return std::nullopt;
  const auto rest = wordsOf(line.substr(gap));
  if (rest.empty() || !(among(rest[0], kParts) || among(rest[0], kAddons)))
    return std::nullopt;
  return rest[0];
}

} // namespace

LineKind classify(std::string_view raw) {
  const auto line = trimRight(raw);
  if (line.empty())
    return LineKind::Blank;
  if (line == "*")
    return LineKind::Trimmed;
  if (line.contains("========") && line.contains("UNKNOWN"))
    return LineKind::Unknown;
  for (auto start : kMarkers)
    if (line.starts_with(start))
      return LineKind::RouteMarker;
  for (auto start : kExplanations)
    if (line.starts_with(start))
      return LineKind::Explanation;
  if (hasCodes(line))
    return LineKind::DictionaryForm;
  if (auto part = leadingPart(line))
    return among(*part, kAddons) ? LineKind::Addon : LineKind::Inflection;
  return LineKind::Meaning;
}

} // namespace tester
