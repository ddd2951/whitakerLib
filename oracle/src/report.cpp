#include "lines.hpp"

namespace lines {

namespace {

struct Tally {
  std::uint64_t spellings = 0, tokens = 0;
};
struct Group {
  std::uint64_t lines = 0, spellings = 0, tokens = 0;
  std::vector<std::string> examples;
};

} // namespace

int run(const fs::path& captures, const fs::path& out, std::vector<std::string> names) {
  std::uint64_t bookTokens = 0;
  const auto spellings = load(captures, names, bookTokens);
  if (!fs::create_directories(out))
    throw std::runtime_error("output must be new: " + out.string());

  std::array<Tally, 3> verdicts{};
  Tally tricks{};
  std::array<Group, std::to_underlying(Rule::Count)> rules{};
  std::map<std::string, Group> differences;
  std::map<std::string, Group> ownDifferences;
  struct FieldTally {
    std::uint64_t equal = 0, differs = 0, gap = 0, spellings = 0, tokens = 0;
  };
  std::map<std::string_view, FieldTally> fields;
  std::vector<std::string_view> fieldOrder;
  // NOTE: Lines with no counterpart, per kind: [0] Ada only, [1] library only.
  std::array<std::array<Group, kKindNames.size()>, 2> lonely{};
  std::uint64_t formsRebuilt = 0;
  std::map<std::string, std::uint64_t> formsDiffering;
  std::vector<std::pair<std::uint64_t, std::string>> listed;
  std::ostringstream unmapped;
  for (const auto& [word, seen] : spellings) {
    const auto s = compare(word, seen.response);
    formsRebuilt += s.formsRebuilt;
    for (const auto& d : s.formsDiffering)
      ++formsDiffering[d];
    auto& v = verdicts[std::to_underlying(s.verdict)];
    ++v.spellings, v.tokens += seen.tokens;
    const auto kinds = [](const std::vector<Line>& lines) {
      return lines | std::views::transform(&Line::kind) | std::ranges::to<std::vector>();
    };
    // INFO: UNKNOWN with "*" where the library answers a plain UNKNOWN: WORDS' guess, all of it trimmed (pergamum is
    //  perg + amum with TRIM_OUTPUT N). The library does everything else WORDS does, so nothing else is left to trim.
    const bool trimmedGuess =
        kinds(s.ada) == std::vector{Kind::Unknown, Kind::Trimmed} && kinds(s.library) == std::vector{Kind::Unknown};
    // NOTE: The library takes WORDS' other routes (Syncope, Word mod, Slur) and prints them, so they are not guesses.
    const bool trick =
        s.verdict == Verdict::Differs &&
        (trimmedGuess || std::ranges::any_of(s.ada, [](const Line& l) {
           return l.kind == Kind::Route && (l.text.starts_with("Two words") || l.text.starts_with("Bad Roman Numeral"));
         }));
    if (trick)
      ++tricks.spellings, tricks.tokens += seen.tokens;
    if (s.verdict != Verdict::Maps)
      unmapped << std::format("{}\t{}\t{}\t{}\n", word, seen.tokens, s.verdict == Verdict::Gaps ? "gaps" : "differs",
                              trick ? "trick" : "-");
    std::set<std::string> ownGroups;
    std::set<Rule> used;
    std::set<std::string> groups;
    std::set<std::string_view> fieldsDiffering;
    std::set<std::pair<int, std::size_t>> lonelySeen;
    for (const auto& p : s.pairs) {
      for (const auto& f : p.fields) {
        if (!fields.contains(f.name))
          fieldOrder.push_back(f.name);
        auto& t = fields[f.name];
        ++(f.status == Status::Equal ? t.equal : f.status == Status::Gap ? t.gap : t.differs);
        if (f.status == Status::Differs && fieldsDiffering.insert(f.name).second)
          ++t.spellings, t.tokens += seen.tokens;
      }
      if (p.differs) {
        auto& g = differences[pattern(p)];
        ++g.lines;
        if (groups.insert(pattern(p)).second) {
          ++g.spellings, g.tokens += seen.tokens;
          if (g.examples.size() < 8)
            g.examples.push_back(word);
        }
        // NOTE: On a trick spelling the trick's are WORDS' lines the library has no counterpart for, and the library's
        //  UNKNOWN; readings the library gives stay its own.
        if (!trick || (p.library && p.library->kind != Kind::Unknown)) {
          auto& o = ownDifferences[pattern(p)];
          ++o.lines;
          if (ownGroups.insert(pattern(p)).second) {
            ++o.spellings, o.tokens += seen.tokens;
            if (o.examples.size() < 8)
              o.examples.push_back(word);
          }
        }
        if (!p.ada || !p.library) {
          const int side = p.ada ? 0 : 1;
          const auto kind = std::to_underlying((p.ada ? p.ada : p.library)->kind);
          auto& l = lonely[side][kind];
          ++l.lines;
          if (lonelySeen.insert({side, kind}).second)
            ++l.spellings, l.tokens += seen.tokens;
        }
        continue;
      }
      for (auto r : p.rules) {
        auto& g = rules[std::to_underlying(r)];
        ++g.lines;
        if (used.insert(r).second) {
          ++g.spellings, g.tokens += seen.tokens;
          if (g.examples.size() < 8)
            g.examples.push_back(word);
        }
      }
    }
    if (s.verdict == Verdict::Differs) {
      std::ostringstream t;
      t << "=== " << word << "  " << seen.tokens << " tokens" << (s.status.empty() ? "" : "  library " + s.status)
        << '\n';
      for (const auto& p : s.pairs) {
        const char mark = p.differs ? '!' : p.rules.empty() ? ' ' : '~';
        std::string notes;
        for (auto r : p.rules)
          notes += std::string(notes.empty() ? "  {" : ", ") + std::string(kRules[std::to_underlying(r)].name);
        if (!notes.empty())
          notes += '}';
        for (const auto& f : p.fields)
          if (f.status == Status::Differs)
            notes += std::format("  [{} differs]", f.name);
        t << std::format("{} {:<60} | {}{}\n", mark, p.ada ? p.ada->text : "", p.library ? p.library->text : "", notes);
      }
      listed.emplace_back(seen.tokens, t.str());
    }
  }

  const auto total = spellings.size();
  const auto pct = [](std::uint64_t part, std::uint64_t whole) { return whole ? 100.0 * part / whole : 0.0; };
  std::ostringstream sum;
  sum << std::format("library {}\ncaptures {} ({} books)\n{} spellings, {} tokens\n\n", WHITAKERLIB_DIR,
                     fs::absolute(captures).string(), names.size(), total, bookTokens);
  constexpr std::array kVerdictNames{"maps: every field WORDS shows is equal", "maps but for gaps", "differs"};
  for (std::size_t i = 0; i < verdicts.size(); ++i)
    sum << std::format("  {:<40} {:>7} spellings ({:6.2f}%)  {:>8} tokens ({:6.2f}%)\n", kVerdictNames[i],
                       verdicts[i].spellings, pct(verdicts[i].spellings, total), verdicts[i].tokens,
                       pct(verdicts[i].tokens, bookTokens));
  sum << std::format("  {:<40} {:>7} spellings ({:6.2f}%)  {:>8} tokens ({:6.2f}%)\n", "  of which WORDS used a trick",
                     tricks.spellings, pct(tricks.spellings, total), tricks.tokens, pct(tricks.tokens, bookTokens));
  sum << std::format("\n{:<24} {:>9} {:>9} {:>12}   {:>9} {:>9}\n", "field (lines)", "equal", "differs", "not returned",
                     "spellings", "tokens");
  for (auto name : fieldOrder) {
    const auto& t = fields[name];
    sum << std::format("  {:<22} {:>9} {:>9} {:>12}   {:>9} {:>9}\n", name, t.equal, t.differs, t.gap, t.spellings,
                       t.tokens);
  }
  sum << std::format("\n{:<24} {:>9} {:>9} {:>9}   {:>9} {:>9} {:>9}\n", "lines without a match", "Ada only",
                     "spellings", "tokens", "lib only", "spellings", "tokens");
  for (std::size_t k = 0; k < kKindNames.size(); ++k) {
    const auto& a = lonely[0][k];
    const auto& l = lonely[1][k];
    if (a.lines || l.lines)
      sum << std::format("  {:<22} {:>9} {:>9} {:>9}   {:>9} {:>9} {:>9}\n", kKindNames[k], a.lines, a.spellings,
                         a.tokens, l.lines, l.spellings, l.tokens);
  }
  for (const auto kind : {"gap"sv, "reading"sv}) {
    sum << (kind == "gap" ? "\ngaps: fields WORDS shows that the library does not return yet\n"
                          : "\nread through WORDS' printing (the same value, not a difference)\n");
    for (std::size_t r = 0; r < rules.size(); ++r)
      if (kRules[r].kind == kind)
        sum << std::format("  {:<18} {:>8} lines {:>7} spellings {:>8} tokens  {}\n", kRules[r].name, rules[r].lines,
                           rules[r].spellings, rules[r].tokens, kRules[r].why);
    if (kind == "reading")
      sum << "  the library gives each record its own codes; WORDS prints one form line for records with the same "
             "dictionary form, so the others' are checked against WORDS' records\n";
  }
  sum << std::format(
      "\nself-check: Dictionary_Form rebuilt from WORDS' record for {} form lines; {} differ ({} distinct)\n",
      formsRebuilt, std::ranges::fold_left(formsDiffering | std::views::values, 0ull, std::plus{}),
      formsDiffering.size());
  sum << "report: " << out.string() << '\n';
  write(out / "forms_rebuilt.txt", [&](std::ostream& f) {
    for (const auto& [d, n] : formsDiffering)
      f << n << '\t' << d << '\n';
  });
  std::cout << sum.str();
  write(out / "summary.txt", [&](std::ostream& f) { f << sum.str(); });

  write(out / "rules.tsv", [&](std::ostream& f) {
    f << "rule\tkind\tlines\tspellings\ttokens\twhy\texamples\n";
    for (std::size_t r = 0; r < rules.size(); ++r) {
      f << kRules[r].name << '\t' << kRules[r].kind << '\t' << rules[r].lines << '\t' << rules[r].spellings << '\t'
        << rules[r].tokens << '\t' << kRules[r].why << '\t';
      for (std::size_t e = 0; e < rules[r].examples.size(); ++e)
        f << (e ? " " : "") << rules[r].examples[e];
      f << '\n';
    }
  });
  for (const auto& [file, groups] :
       {std::pair{"differences.tsv", &differences}, {"differences_own.tsv", &ownDifferences}}) {
    std::vector<std::pair<std::string, Group>> sorted(groups->begin(), groups->end());
    std::ranges::stable_sort(sorted, std::greater{}, [](const auto& p) { return p.second.tokens; });
    write(out / file, [&](std::ostream& f) {
      f << "tokens\tspellings\tlines\tdifference\texamples\n";
      for (const auto& [k, g] : sorted) {
        f << g.tokens << '\t' << g.spellings << '\t' << g.lines << '\t' << k << '\t';
        for (std::size_t e = 0; e < g.examples.size(); ++e)
          f << (e ? " " : "") << g.examples[e];
        f << '\n';
      }
    });
  }
  std::ranges::stable_sort(listed, std::greater{}, &std::pair<std::uint64_t, std::string>::first);
  write(out / "lines.txt", [&](std::ostream& f) {
    f << "! differs   ~ maps through the rules in {}   left: Ada as printed   right: the library laid out as WORDS "
         "would\n\n";
    for (const auto& [t, text] : listed)
      f << text << '\n';
  });
  write(out / "unmapped.tsv", [&](std::ostream& f) { f << unmapped.str(); });
  // NOTE: Either one leaves the figures above without a sure reference; the wrapper then keeps the book list as it was.
  if (!formsDiffering.empty()) {
    std::cerr << "self-check failed: the Dictionary_Form rebuild differs, see forms_rebuilt.txt\n";
    return 1;
  }
  return 0;
}

} // namespace lines
