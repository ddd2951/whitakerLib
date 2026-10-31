#include "lines.hpp"

namespace lines {

namespace {

std::string fieldsOf(const Line& l, bool library) {
  std::string out = std::format("{:<10}", kKindNames[std::to_underlying(l.kind)]);
  const auto add = [&](std::string_view name, std::string_view value) {
    if (!value.empty())
      out += std::format(" {}=\"{}\"", name, value);
  };
  switch (l.kind) {
  case Kind::Inflection: {
    add("stem", l.stem);
    add("ending", l.knownEnding ? l.ending : "?");
    const auto q = qualifierFields(l.qual);
    add("part", q[0]), add("decl/conj", q[1]), add("grammar", q[2]), add("labels", l.labels);
    if (l.deponent)
      out += " deponent";
    break;
  }
  case Kind::Form:
    add("parts", library && !l.knownParts ? "?" : l.parts), add("form", l.qual), add("codes", l.codes);
    add("ref", library && !l.knownRef ? "?" : l.ref), add("labels", l.labels);
    if (l.unique)
      out += " unique";
    break;
  case Kind::Addon:
    add("addon", l.stem), add("kind", l.qual);
    break;
  case Kind::Meaning:
    add("meaning", l.qual);
    break;
  case Kind::Unknown:
    add("word", l.stem);
    break;
  default:
    add("text", l.text);
    break;
  }
  return out;
}

} // namespace

int show(const fs::path& captures, const std::vector<std::string>& words) {
  std::vector<std::string> names;
  std::uint64_t bookTokens = 0;
  const auto spellings = load(captures, names, bookTokens, true);
  for (const auto& word : words) {
    const auto seen = spellings.find(word);
    std::cout << "######## " << word << '\n';
    if (seen == spellings.end()) {
      std::cout << "not in the captures\n\n";
      continue;
    }
    const auto s = compare(word, seen->second.response);
    constexpr std::array kVerdictNames{"maps", "maps but for gaps", "differs"};
    std::cout << std::format("{} tokens, verdict: {}{}\n\n--- Ada as printed\n{}", seen->second.tokens,
                             kVerdictNames[std::to_underlying(s.verdict)],
                             s.status.empty() ? "" : ", library " + s.status, seen->second.response);
    std::cout << "--- Ada, split into fields\n";
    for (std::size_t i = 0; i < s.ada.size(); ++i)
      std::cout << std::format("A{:<3} {}\n", i, fieldsOf(s.ada[i], false));
    std::cout << "--- the library, laid out as WORDS would (? = not returned)\n";
    for (std::size_t i = 0; i < s.library.size(); ++i)
      std::cout << std::format("L{:<3} {}\n", i, fieldsOf(s.library[i], true));
    std::cout << "--- pairs (= equal, ! differs, ~ gap; {rules})\n";
    const auto at = [](const auto& all, const Line* l, char side) {
      return l ? std::format("{}{}", side, l - all.data()) : std::string("-");
    };
    for (const auto& p : s.pairs) {
      std::string line = std::format("{:<5} {:<5} {}", at(s.ada, p.ada, 'A'), at(s.library, p.library, 'L'),
                                     p.differs ? "DIFFERS" : "maps   ");
      for (const auto& f : p.fields) {
        if (f.status == Status::Equal)
          line += std::format(" ={}", f.name);
        else if (f.status == Status::Gap)
          line += std::format(" ~{}", f.name);
        else
          line += std::format(" !{}(Ada \"{}\" lib \"{}\")", f.name, f.ada, f.library);
      }
      std::string rs;
      for (auto r : p.rules)
        rs += std::string(rs.empty() ? "  {" : ", ") + std::string(kRules[std::to_underlying(r)].name);
      std::cout << line << rs << (rs.empty() ? "" : "}") << '\n';
    }
    std::cout << '\n';
  }
  return 0;
}

} // namespace lines
