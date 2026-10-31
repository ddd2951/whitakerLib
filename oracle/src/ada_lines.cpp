#include "lines.hpp"

namespace lines {

namespace {

using tester::LineKind;

constexpr std::array kParts{"N"sv,    "V"sv,    "ADJ"sv,    "ADV"sv,  "PRON"sv, "VPAR"sv,   "SUPINE"sv, "NUM"sv,
                            "PREP"sv, "CONJ"sv, "INTERJ"sv, "PACK"sv, "X"sv,    "TACKON"sv, "PREFIX"sv, "SUFFIX"sv};
constexpr std::array kInflectionLabels{"Archaic"sv,  "Early"sv,   "Classic"sv, "Late"sv,     "Later"sv,
                                       "Medieval"sv, "Scholar"sv, "Modern"sv,  "mostfreq"sv, "sometime"sv,
                                       "uncommon"sv, "infreq"sv,  "rare"sv,    "veryrare"sv, "inscript"sv};

// INFO: "am.o  V  1 1 PRES ACTIVE IND 1 S  Early". A stem and ending of 21 characters or more run into the part
//  ("conperendination.ibusN"), since WORDS sets the qualifier's column after them.
std::optional<Line> inflectionLine(std::string_view raw) {
  auto words = tester::wordsOf(raw);
  if (words.size() < 2 || raw.front() == ' ')
    return std::nullopt;
  std::string first(words[0]);
  std::vector<std::string> rest(words.begin() + 1, words.end());
  if (std::ranges::contains(kParts, std::string_view(rest[0]))) {
    // INFO: Put_Inflection sets the part's column to 22: a meaning whose second word is a part name is no reading.
    if (first.size() > 20 || raw.size() <= 21 || !raw.substr(21).starts_with(rest[0]))
      return std::nullopt;
  } else {
    if (first.size() < 21)
      return std::nullopt;
    const auto glued = std::ranges::find_if(kParts, [&](auto p) {
      return first.ends_with(p) && first.size() > p.size() && std::islower(first[first.size() - p.size() - 1]);
    });
    if (glued == kParts.end())
      return std::nullopt;
    rest.insert(rest.begin(), std::string(*glued));
    first.resize(first.size() - glued->size());
  }
  Line l;
  const bool addon = rest[0] == "TACKON" || rest[0] == "PREFIX" || rest[0] == "SUFFIX";
  l.kind = addon ? Kind::Addon : Kind::Inflection;
  const auto dot = first.find('.');
  l.stem = first.substr(0, dot);
  if (dot != std::string::npos)
    l.ending = first.substr(dot + 1);
  while (!addon && rest.size() > 1 && std::ranges::contains(kInflectionLabels, std::string_view(rest.back())))
    l.labels = rest.back() + (l.labels.empty() ? "" : " ") + l.labels, rest.pop_back();
  for (const auto& w : rest)
    l.qual += (l.qual.empty() ? "" : " ") + w;
  return l;
}

Line formLine(std::string_view raw) {
  Line l;
  l.kind = Kind::Form;
  const auto open = raw.find('[');
  auto before = raw.substr(0, open);
  while (!before.empty() && before.back() == ' ')
    before.remove_suffix(1);
  if (const auto gap = before.find("  "); gap != std::string_view::npos) {
    l.parts = collapsed(before.substr(0, gap));
    l.qual = collapsed(before.substr(gap));
  } else {
    // INFO: "qu  PACK" has a gap; a lone word is all parts
    l.parts = collapsed(before);
  }
  l.printedForm = collapsed(before);
  l.codes = std::string(raw.substr(open + 1, 5));
  const auto after = raw.substr(open + 7);
  const auto close = after.find(')');
  l.ref = collapsed(after.substr(0, close == std::string_view::npos ? 0 : close + 1));
  l.unique = l.ref.starts_with("UNI>");
  if (close != std::string_view::npos)
    l.labels = collapsed(after.substr(close + 1));
  return l;
}

} // namespace

std::vector<Line> adaLines(std::string_view response) {
  std::vector<Line> out;
  for (auto piece : response | std::views::split('\n')) {
    std::string_view raw(piece.begin(), piece.end());
    while (!raw.empty() && (raw.back() == ' ' || raw.back() == '\r'))
      raw.remove_suffix(1);
    if (raw.empty())
      continue;
    Line l;
    switch (tester::classify(raw)) {
    case LineKind::Blank:
      continue;
    case LineKind::Trimmed:
      l.kind = Kind::Trimmed;
      break;
    case LineKind::Unknown:
      l.kind = Kind::Unknown;
      l.stem = std::string(tester::wordsOf(raw)[0]);
      break;
    case LineKind::RouteMarker:
    case LineKind::Explanation:
      l.kind = Kind::Route;
      break;
    case LineKind::DictionaryForm:
      l = formLine(raw);
      break;
    default:
      if (auto i = inflectionLine(raw))
        l = *i;
      else
        l.kind = Kind::Meaning, l.qual = collapsed(raw);
    }
    l.text = collapsed(raw);
    out.push_back(std::move(l));
  }
  return out;
}

} // namespace lines
