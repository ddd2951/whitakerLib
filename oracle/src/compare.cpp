#include "lines.hpp"

namespace lines {

namespace {

constexpr std::array kCodeNames{"age"sv, "area"sv, "geography"sv, "frequency"sv, "source"sv};

bool digits(std::string_view s) {
  return !s.empty() && std::ranges::all_of(s, [](unsigned char c) { return std::isdigit(c); });
}

} // namespace

std::array<std::string, 3> qualifierFields(std::string_view qual) {
  const auto w = tester::wordsOf(qual);
  std::array<std::string, 3> out;
  if (w.empty())
    return out;
  out[0] = std::string(w[0]);
  std::size_t rest = 1;
  if (w.size() > 2 && digits(w[1]) && digits(w[2]))
    out[1] = std::string(w[1]) + ' ' + std::string(w[2]), rest = 3;
  for (auto i = rest; i < w.size(); ++i)
    out[2] += (out[2].empty() ? "" : " ") + std::string(w[i]);
  return out;
}

namespace {

std::vector<Field> compareFields(const Line& a, const Line& l, Rules& rules) {
  std::vector<Field> f;
  const auto put = [&](std::string_view name, std::string_view x, std::string_view y, bool equal) {
    f.push_back(
        {name, equal ? Status::Equal : Status::Differs, equal ? "" : std::string(x), equal ? "" : std::string(y)});
  };
  const auto gap = [&](std::string_view name, Rule r) {
    f.push_back({name, Status::Gap, {}, {}});
    rules.insert(r);
  };
  const auto spelled = [&](std::string_view name, std::string_view x, std::string_view y) {
    bool equal = x == y;
    if (!equal && tester::folded(x) == tester::folded(y))
      equal = true, rules.insert(Rule::SpellingFold);
    put(name, x, y, equal);
  };
  switch (a.kind) {
  case Kind::Inflection: {
    spelled("stem", a.stem, l.stem);
    if (!l.knownEnding) {
      if (!a.ending.empty())
        gap("ending", Rule::Ending);
    } else {
      spelled("ending", a.ending, l.ending);
    }
    auto q = a.qual, lib = l.qual;
    // INFO: Print_Modified_Qual: a deponent's voice field is blank in IND .. INF and in a participle, whatever it holds
    //  (ACTIVE for a future participle): only where WORDS shows no voice and the mood is one of those.
    if (l.deponent && !q.contains("ACTIVE") && !q.contains("PASSIVE")) {
      const auto words = tester::wordsOf(lib);
      const bool blanked = std::ranges::any_of(
          words, [](std::string_view w) { return w == "IND" || w == "SUB" || w == "IMP" || w == "INF" || w == "PPL"; });
      for (const auto voice : {" ACTIVE"sv, " PASSIVE"sv})
        if (const auto at = lib.find(voice); blanked && at != std::string::npos) {
          lib.erase(at, voice.size()), rules.insert(Rule::DeponentVoice);
          break;
        }
    }
    // INFO: WORDS prints a verb's person 0 (INF 0 X, MANE's X X X 0 X); whitaker_describe leaves it out. After the
    //  deponent's voice, so the two words line up.
    if (const auto w = tester::wordsOf(q), v = tester::wordsOf(lib);
        !w.empty() && w[0] == "V" && w.size() >= 2 && w[w.size() - 2] == "0" && v.size() + 1 == w.size()) {
      std::string without;
      for (std::size_t i = 0; i < w.size(); ++i)
        if (i != w.size() - 2)
          without += (without.empty() ? "" : " ") + std::string(w[i]);
      q = without, rules.insert(Rule::PersonZero);
    }
    const auto x = qualifierFields(q), y = qualifierFields(lib);
    // NOTE: The library names the part twice, in pos and in its grammar; both must be WORDS'.
    put("part", x[0], l.pos == y[0] ? y[0] : y[0] + " pos " + l.pos, x[0] == y[0] && l.pos == y[0]);
    put("declension/conjugation", x[1], y[1], x[1] == y[1]);
    put("grammar", x[2], y[2], x[2] == y[2]);
    put("inflection labels", a.labels, l.labels, a.labels == l.labels);
    break;
  }
  case Kind::Addon:
    spelled("addon", a.stem, l.stem);
    put("addon kind", a.qual, l.qual, a.qual == l.qual);
    break;
  case Kind::Form:
    put("form", a.qual, l.qual, a.qual == l.qual);
    for (std::size_t i = 0; i < kCodeNames.size(); ++i) {
      const auto x = a.codes.substr(std::min(i, a.codes.size()), 1), y = l.codes.substr(std::min(i, l.codes.size()), 1);
      put(kCodeNames[i], x, y, x == y);
    }
    put("entry labels", a.labels, l.labels, a.labels == l.labels);
    if (!a.parts.empty() && !l.knownParts)
      gap("principal parts", Rule::PrincipalParts);
    else
      put("principal parts", a.parts, l.parts, a.parts == l.parts);
    if (!a.ref.empty() && !l.knownRef)
      gap("dictionary ref", Rule::DictionaryRef);
    else
      put("dictionary ref", a.ref, l.ref, a.ref == l.ref);
    break;
  case Kind::Meaning: {
    // INFO: Put_Meaning: Trim (Head (Trim_Bar (Raw), 79)): the bars and blanks off the front, cut at 79, trimmed.
    std::string_view lib = l.raw;
    bool bar = false;
    while (lib.starts_with('|'))
      lib.remove_prefix(1), bar = true;
    while (lib.starts_with(' '))
      lib.remove_prefix(1);
    const bool cut = collapsed(lib.substr(std::min<std::size_t>(lib.size(), 79))).size() > 0;
    const auto shown = collapsed(lib.substr(0, 79));
    const bool equal = shown == a.qual;
    if (equal && bar)
      rules.insert(Rule::MeaningBar);
    if (equal && cut)
      rules.insert(Rule::MeaningWidth);
    put(l.numeral ? "numeral meaning" : "meaning", a.qual, l.qual, equal);
    break;
  }
  // INFO: Do_Qvae_Kludge: WORDS echoes an unknown qv word as qu (noviqvie).
  case Kind::Unknown:
    spelled("unknown", a.stem, l.stem);
    break;
  case Kind::Route:
    put("route", a.text, l.text, a.text == l.text);
    break;
  case Kind::Trimmed:
    put("trimmed", "*", "*", true);
    break;
  default:
    break;
  }
  return f;
}

std::optional<Rules> sameLine(const Line& a, const Line& l) {
  if (a.kind != l.kind)
    return std::nullopt;
  Rules rules;
  for (const auto& f : compareFields(a, l, rules))
    if (f.status == Status::Differs)
      return std::nullopt;
  return rules;
}

} // namespace

Spelling compare(const std::string& word, std::string_view response, const Mutation* mutate, std::size_t only) {
  Spelling s;
  s.word = word;
  s.ada = adaLines(response);
  s.library = libraryLines(word, s.status, mutate, only);
  for (const auto& a : s.ada) {
    if (a.kind != Kind::Form)
      continue;
    unsigned n = 0;
    const bool general = a.ref.starts_with("GEN>("), unique = a.ref.starts_with("UNI>(");
    if (!(general || unique) || std::sscanf(a.ref.c_str() + 5, "%u", &n) != 1 || n == 0)
      continue;
    const auto& dictionary = wordsDictionary();
    const auto* record = dictionary.record(static_cast<std::uint16_t>(unique ? dictionary.general() + n - 1 : n - 1));
    ++s.formsRebuilt;
    if (const auto rebuilt = record ? collapsed(dictionaryForm(*record)) : "?"; rebuilt != a.printedForm)
      s.formsDiffering.push_back(std::format("{}: WORDS \"{}\" rebuilt \"{}\"", a.ref, a.printedForm, rebuilt));
  }
  const auto n = s.ada.size(), m = s.library.size();
  std::vector<std::vector<std::optional<Rules>>> same(n, std::vector<std::optional<Rules>>(m));
  std::vector<std::vector<std::uint32_t>> best(n + 1, std::vector<std::uint32_t>(m + 1));
  for (std::size_t i = n; i-- > 0;)
    for (std::size_t j = m; j-- > 0;) {
      same[i][j] = sameLine(s.ada[i], s.library[j]);
      best[i][j] = same[i][j] ? best[i + 1][j + 1] + 1 : std::max(best[i + 1][j], best[i][j + 1]);
    }
  std::vector<Pairing> pairs;
  std::size_t i = 0, j = 0;
  while (i < n || j < m) {
    if (i < n && j < m && same[i][j] && best[i][j] == best[i + 1][j + 1] + 1) {
      Pairing p{&s.ada[i], &s.library[j], {}, {}, false};
      p.fields = compareFields(*p.ada, *p.library, p.rules);
      pairs.push_back(std::move(p));
      ++i, ++j;
    } else if (j == m || (i < n && best[i + 1][j] >= best[i][j + 1])) {
      Pairing p{&s.ada[i++], nullptr, {}, {}, false};
      if (p.ada->kind == Kind::Route)
        p.rules.insert(Rule::Route);
      else
        p.differs = true;
      pairs.push_back(std::move(p));
    } else {
      pairs.push_back({nullptr, &s.library[j++], {}, {}, true});
    }
  }
  for (std::size_t k = 0; k < pairs.size();) {
    if (!pairs[k].differs) {
      s.pairs.push_back(std::move(pairs[k++]));
      continue;
    }
    auto end = k;
    while (end < pairs.size() && pairs[end].differs)
      ++end;
    std::vector<std::size_t> left;
    for (auto x = k; x < end; ++x)
      if (!pairs[x].ada)
        left.push_back(x);
    std::size_t next = 0;
    for (auto x = k; x < end; ++x) {
      auto& p = pairs[x];
      if (!p.ada)
        continue;
      const auto it = std::find_if(left.begin() + next, left.end(),
                                   [&](std::size_t y) { return pairs[y].library->kind == p.ada->kind; });
      if (it != left.end()) {
        Rules ignored;
        p.library = pairs[*it].library;
        p.fields = compareFields(*p.ada, *p.library, ignored);
        pairs[*it].library = nullptr;
        next = static_cast<std::size_t>(it - left.begin()) + 1;
      }
      s.pairs.push_back(std::move(p));
    }
    for (auto y : left)
      if (pairs[y].library)
        s.pairs.push_back(std::move(pairs[y]));
    k = end;
  }
  // INFO: A route's marker and explanation are an entry of their own in WORDS' layout, with no dictionary form, so the
  //  form line after them is printed again. The library returns no route (a gap), so its layout cannot repeat it: an
  //  Ada form line the same as the one before the route lines is the route's, not a difference.
  for (auto& p : s.pairs) {
    if (!p.differs || p.library || p.ada->kind != Kind::Form)
      continue;
    bool route = false;
    for (auto k = static_cast<std::size_t>(p.ada - s.ada.data()); k-- > 0;) {
      if (s.ada[k].kind == Kind::Route)
        route = true;
      else if (s.ada[k].kind == Kind::Form) {
        if (route && s.ada[k].printedForm == p.ada->printedForm && s.ada[k].codes == p.ada->codes &&
            s.ada[k].ref == p.ada->ref)
          p.differs = false, p.rules.insert(Rule::Route);
        break;
      }
    }
  }
  const auto once = [&](const std::vector<Line>& all, auto side) {
    std::vector<int> seen(all.size());
    for (const auto& p : s.pairs)
      if (const Line* l = p.*side)
        ++seen[static_cast<std::size_t>(l - all.data())];
    return std::ranges::all_of(seen, [](int n) { return n == 1; });
  };
  if (!once(s.ada, &Pairing::ada) || !once(s.library, &Pairing::library))
    throw std::logic_error("a line is not in exactly one pairing: " + word);
  for (const auto& p : s.pairs) {
    if (p.differs)
      s.verdict = Verdict::Differs;
    for (auto r : p.rules)
      if (kRules[std::to_underlying(r)].kind != "reading")
        s.verdict = std::max(s.verdict, Verdict::Gaps);
  }
  return s;
}

std::string pattern(const Pairing& p) {
  if (p.ada && p.library) {
    std::string out = std::format("{}:", kKindNames[std::to_underlying(p.ada->kind)]);
    for (const auto& f : p.fields)
      if (f.status == Status::Differs)
        out += std::format(" {} -{} +{};", f.name, f.ada, f.library);
    return out;
  }
  const auto& l = p.ada ? *p.ada : *p.library;
  const std::string side = p.ada ? "Ada only" : "library only";
  switch (l.kind) {
  case Kind::Inflection:
    return std::format("{} reading: {}", side, l.qual);
  case Kind::Form:
    return std::format("{} form: {} [{}] {}", side, l.qual, l.codes, l.labels);
  case Kind::Meaning:
    return std::format("{} meaning: {}", side, l.qual);
  case Kind::Addon:
    return std::format("{} addon: {} {}", side, l.stem, l.qual);
  default:
    return std::format("{} {}", side, kKindNames[std::to_underlying(l.kind)]);
  }
}

} // namespace lines
