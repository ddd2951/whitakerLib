#include "lines.hpp"

namespace lines {

namespace {

template <typename E> std::string name(const std::uint8_t value) {
  const char* const n = latin::name(E{value});
  return n ? n : "?";
}

// INFO: Route codes include the syncope rule from ReadingDetail's derivation.
inline constexpr std::uint8_t kRouteDirect = 0, kRouteSyncopeII = 1, kRouteSyncopeS = 2, kRouteSyncopeR = 3,
                              kRouteSyncopeIer = 4, kRouteSyncopeIs = 5, kRouteAdverb = 6, kRouteSlur = 7,
                              kRouteWordMod = 8;

static_assert(kRouteSyncopeII == std::to_underlying(latin::Syncope::II) &&
              kRouteSyncopeIs == std::to_underlying(latin::Syncope::IS));

[[nodiscard]] std::uint8_t oracleRoute(const whitaker::ReadingDetail& d) {
  switch (d.route) {
    case latin::Route::direct:
      return kRouteDirect;
    case latin::Route::syncope:
      return std::to_underlying(d.form.derivation.syncope);
    case latin::Route::adverb:
      return kRouteAdverb;
    case latin::Route::slur:
      return kRouteSlur;
    case latin::Route::wordMod:
      return kRouteWordMod;
  }
  return kRouteDirect;
}

// INFO: Support_Utils.Dictionary_Form after the principal parts, from the library's entry.
std::string libraryForm(const WhitakerEntry& e, const std::string_view grammar) {
  static constexpr std::array ordinals{"", "1st", "2nd", "3rd", "4th", "5th"};
  const auto part = name<latin::Part>(e.part);
  // INFO: A unique has one stem: "stem  PART", but for the forms Dictionary_Form builds from one stem.
  const auto comparison = name<latin::Comparison>(e.comparison);
  if (e.id >= std::to_underlying(whitaker::kFirstUnique) && e.id != std::to_underlying(whitaker::kNoDictionaryWord) &&
      !((part == "N" && e.which == 9) ||
        (part == "ADJ" && (e.which == 9 || comparison == "COMP" || comparison == "SUPER")) ||
        (part == "V" && e.which == 9 && (e.variant == 8 || e.variant == 9))))
    return part;
  if (part == "PREP")
    return std::string(grammar);
  if (part == "PRON" && !std::ranges::contains(std::array{3, 4, 6, 9}, +e.which))
    return "";
  auto out = part;
  if (part == "N") {
    if (e.which >= 1 && e.which <= 5 && e.variant >= 1 && e.variant <= 5)
      out += std::format(" ({})", ordinals[e.which]);
    out += ' ' + name<latin::Gender>(e.gender);
  } else if (part == "V") {
    if (e.which >= 1 && e.which <= 3) {
      if (e.variant == 1)
        out += std::format(" ({})", ordinals[e.which]);
      else if (e.which == 3 && e.variant == 4)
        out += " (4th)";
    }
    if (const auto k = name<latin::VerbKind>(e.kind); std::ranges::contains(
            std::array{"GEN", "DAT", "ABL", "TRANS", "INTRANS", "IMPERS", "DEP", "SEMIDEP", "PERFDEF"}, k))
      out += ' ' + k;
  }
  return out;
}

// INFO: Put_Dictionary_Flags: the entry's age unless X, its frequency from D on.
std::string dictionaryLabels(const WhitakerEntry& e) {
  static const std::map<std::string, std::string> ageLabels{{"A", "Archaic"},  {"B", "Early"}, {"C", "Classic"},
                                                            {"D", "Late"},     {"E", "Later"}, {"F", "Medieval"},
                                                            {"G", "NeoLatin"}, {"H", "Modern"}};
  static const std::map<std::string, std::string> frequencyLabels{
      {"D", "lesser"}, {"E", "uncommon"}, {"F", "veryrare"}, {"I", "inscript"}, {"J", "graffiti"}, {"N", "Pliny"}};
  std::string out;
  if (const auto a = ageLabels.find(name<latin::Age>(e.age)); a != ageLabels.end())
    out = a->second;
  if (const auto f = frequencyLabels.find(name<latin::Frequency>(e.frequency)); f != frequencyLabels.end())
    out += (out.empty() ? "" : " ") + f->second;
  return out;
}

// INFO: Put_Inflection_Flags prints age unless X and frequency from C on (list_package.adb).
std::string inflectionLabels(const std::uint8_t age, const std::uint8_t frequency) {
  static const std::map<std::string, std::string> ageLabels{{"A", "Archaic"}, {"B", "Early"}, {"C", "Classic"},
                                                            {"D", "Late"},    {"E", "Later"}, {"F", "Medieval"},
                                                            {"G", "Scholar"}, {"H", "Modern"}};
  static const std::map<std::string, std::string> frequencyLabels{
      {"C", "uncommon"}, {"D", "infreq"}, {"E", "rare"}, {"F", "veryrare"}, {"I", "inscript"}};
  std::string out;
  if (const auto a = ageLabels.find(name<latin::Age>(age)); a != ageLabels.end())
    out = a->second;
  if (const auto f = frequencyLabels.find(name<latin::Frequency>(frequency)); f != frequencyLabels.end())
    out += (out.empty() ? "" : " ") + f->second;
  return out;
}

// INFO: Constructed_Meaning (list_package.adb): what WORDS prints for a numeral with a value instead of its meaning,
//  from the value and the first reading's part and sort; Integer'Image puts a blank before the number.
std::string constructedMeaning(const unsigned value, const std::string_view part, const std::string_view sort) {
  const auto n = std::format(" {}", value);
  std::string out;
  if (part != "NUM")
    out = "Number " + n;
  else if (sort == "CARD")
    out = n + " - (CARD answers 'how many');";
  else if (sort == "ORD")
    out = n + "th - (ORD, 'in series'); (a/the)" + n + "th (part) (fract w/pars?);";
  else if (sort == "DIST")
    out = n + " each/apiece/times/fold/together/at a time - 'how many each'; by " + n + "s; ";
  else if (sort == "ADVERB")
    out = n + " times, on" + n + " occasions - (ADVERB answers 'how often');";
  return collapsed(std::string_view(out).substr(0, 80));
}

// INFO: latin::describe's "N D2 V2 NOM P N" as WORDS' qualifier "N 2 2 NOM P N".
std::string qualifier(const std::string_view described) {
  const auto words = tester::wordsOf(described);
  std::string out;
  for (std::size_t i = 0; i < words.size(); ++i) {
    auto w = words[i];
    if ((i == 1 && (w.starts_with('D') || w.starts_with('C')) && w.size() > 1 && std::isdigit(w[1])) ||
        (i == 2 && w.starts_with('V') && w.size() > 1 && std::isdigit(w[1])))
      w.remove_prefix(1);
    out += (out.empty() ? "" : " ") + std::string(w);
  }
  return out;
}

const char* printedKind(const std::uint8_t kind) {
  switch (kind) {
    case std::to_underlying(latin::AddonKind::prefix):
    case std::to_underlying(latin::AddonKind::tickon):
      return "PREFIX";
    case std::to_underlying(latin::AddonKind::suffix):
      return "SUFFIX";
    // INFO: WORDS prints a packon on a TACKON line.
    default:
      return "TACKON";
  }
}

Line meaningLine(const std::string_view meaning) {
  Line l;
  l.kind = Kind::Meaning;
  l.raw = std::string(meaning);
  l.qual = collapsed(meaning);
  l.text = l.qual;
  return l;
}

} // namespace

const Dictionary& wordsDictionary() {
  static const Dictionary dictionary = [] {
    const fs::path words = ADA_DATA_DIR, library = fs::path(WHITAKERLIB_DIR) / "data/source";
    for (const auto* const file : {"DICTLINE.GEN", "UNIQUES.LAT"}) {
      std::ifstream a(words / file, std::ios::binary), b(library / file, std::ios::binary);
      if (!a || !b || !std::equal(std::istreambuf_iterator<char>(a), {}, std::istreambuf_iterator<char>(b), {}))
        throw std::runtime_error(std::format("WORDS' {} is not the library's", file));
    }
    return Dictionary(words);
  }();
  return dictionary;
}

std::vector<Line> libraryLines(const std::string& word, std::string& status, const Mutation* const mutate,
                               const std::size_t only) {
  thread_local whitaker::Result result;
  if (const auto s = whitaker::analyze(result, word); s != whitaker::Status::ok) {
    status = s == whitaker::Status::inputTooLong ? "InputTooLong" : std::format("status {}", +std::to_underlying(s));
    return {};
  }
  const whitaker::WordId analyzed{0};
  const auto readings = whitaker::readings(result, analyzed);
  // INFO: A record is a run of readings with one entry, part, route, spelling rule and add-ons.
  const auto sameRecord = [](const whitaker::ReadingDetail& a, const whitaker::ReadingDetail& b) {
    return a.dictionaryWordId == b.dictionaryWordId && a.form.grammar.part == b.form.grammar.part &&
           a.route == b.route && a.form.spellingRuleId == b.form.spellingRuleId && a.addonCount == b.addonCount &&
           std::ranges::equal(std::span(a.addons, a.addonCount), std::span(b.addons, b.addonCount),
                              [](const auto& x, const auto& y) { return x.id == y.id && x.kind == y.kind; });
  };
  // NOTE: Copies of the records, their add-on steps and their forms, which --probe changes; one spare zero step past
  //  each count, as a raised addon_count reads it. The details hold the text the copies point at.
  std::vector<whitaker::ReadingDetail> details(readings.size());
  std::vector<WhitakerRecord> records;
  std::vector<WhitakerForm> forms;
  std::vector<std::vector<WhitakerAddonStep>> steps;
  for (std::size_t i = 0; i < readings.size(); ++i) {
    auto& d = details[i];
    d = whitaker::decode(result, analyzed, readings[i]);
    const auto& e = d.dictionaryWord;
    const auto& f = d.form;
    const auto& g = f.grammar;
    if (i == 0 || !sameRecord(details[i - 1], d)) {
      records.push_back(
          {.entry = {std::to_underlying(e.part), e.which.value, e.variant.value, std::to_underlying(e.gender),
                     e.kind.value, std::to_underlying(e.comparison), std::to_underlying(e.numeralSort),
                     std::to_underlying(e.area), std::to_underlying(e.geography), std::to_underlying(e.source),
                     e.numeralValue.value, std::to_underlying(e.age), std::to_underlying(e.frequency),
                     std::to_underlying(d.dictionaryWordId)},
           .meaning = d.meaning,
           .pos = d.partName,
           .addons = nullptr,
           .addon_count = d.addonCount,
           .route = oracleRoute(d),
           .spelling_rule = f.spellingRuleId,
           .first_form = i,
           .form_count = 0});
      steps.emplace_back();
      for (unsigned a = 0; a < d.addonCount; ++a)
        steps.back().push_back({d.addons[a].id, std::to_underlying(d.addons[a].kind)});
      steps.back().push_back({});
    }
    ++records.back().form_count;
    forms.push_back({f.stemSpelling,
                     f.ending,
                     {std::to_underlying(g.part), g.which.value, g.variant.value, std::to_underlying(g.caseOf),
                      std::to_underlying(g.number), std::to_underlying(g.gender), std::to_underlying(g.comparison),
                      std::to_underlying(g.numeralSort), std::to_underlying(g.tense), std::to_underlying(g.voice),
                      std::to_underlying(g.mood), g.person.value},
                     std::to_underlying(f.inflectionAge),
                     std::to_underlying(f.inflectionFrequency)});
  }
  for (std::size_t k = 0; k < records.size(); ++k)
    records[k].addons = steps[k].data();
  if (mutate && mutate->record && only == kAll)
    std::ranges::for_each(records, mutate->record);
  // NOTE: One record changed alone: its entry goes to every record of its id, as the library gives one id one entry.
  if (mutate && mutate->record && only < records.size()) {
    const auto id = records[only].entry.id;
    mutate->record(records[only]);
    for (auto& r : records)
      if (r.entry.id == id)
        r.entry = records[only].entry;
  }
  if (mutate && mutate->form && only == kAll)
    std::ranges::for_each(forms, mutate->form);
  if (mutate && mutate->form && only < forms.size())
    mutate->form(forms[only]);
  std::vector<Line> out;
  if (records.empty()) {
    Line l;
    l.kind = Kind::Unknown, l.stem = word, l.text = word + " ======== UNKNOWN";
    if (!whitaker::trimmed(result, analyzed))
      return {l};
    Line star;
    star.kind = Kind::Trimmed, star.text = "*";
    return {l, star};
  }
  // INFO: WhitakerWord::trimmed: WORDS' "*" after the answer (List_Stems).
  const bool trimmed = whitaker::trimmed(result, analyzed);
  struct Entry {
    std::vector<Line> readings;
    const WhitakerRecord* first = nullptr;
    const WhitakerForm* firstForm = nullptr;
    const Record* record = nullptr;
    std::string form;
    std::vector<std::pair<std::string, std::string>> addons;
    // INFO: WORDS prints a line for each ADDONS record: two records that read the same (ell, ell) print twice
    std::vector<std::uint16_t> addonIds;
    bool roman = false;
    std::uint8_t route = kRouteDirect;
  };
  std::vector<Entry> entries;
  std::map<std::uint16_t, WhitakerEntry> byId;
  std::array<char, 256> described{};
  for (const WhitakerRecord& m : records) {
    if (const auto [at, added] = byId.try_emplace(m.entry.id, m.entry);
        !added && std::memcmp(&at->second, &m.entry, sizeof m.entry) != 0)
      throw std::logic_error("records of one id with different entry fields: " + word);
    const std::size_t begin = m.first_form;
    const std::size_t end = m.first_form + m.form_count;
    Entry e;
    e.first = &m;
    e.firstForm = begin < end && begin < forms.size() ? &forms[begin] : nullptr;
    for (int a = 0; a < m.addon_count; ++a) {
      const char* const s = whitaker::addonSpelling(m.addons[a].id);
      const char* const mean = whitaker::addonMeaning(m.addons[a].id);
      e.addons.emplace_back(std::string(s ? s : "?") + ' ' + printedKind(m.addons[a].kind), mean ? mean : "");
      e.addonIds.push_back(m.addons[a].id);
    }
    e.roman = m.entry.id == std::to_underlying(whitaker::kNoDictionaryWord);
    e.route = m.route;
    e.record = wordsDictionary().record(m.entry.id);
    if (e.record)
      e.form = dictionaryForm(*e.record);
    for (std::size_t f = begin; f < end; ++f) {
      if (f >= forms.size()) {
        Line l;
        l.kind = Kind::Inflection, l.text = "(a form past the end)";
        e.readings.push_back(std::move(l));
        continue;
      }
      const WhitakerForm& form = forms[f];
      const auto& g = form.grammar;
      latin::describe(latin::Analysis{.part = latin::Part{g.part},
                                      .which = g.which,
                                      .variant = {g.variant},
                                      .caseOf = latin::Case{g.case_of},
                                      .number = latin::Number{g.number},
                                      .gender = latin::Gender{g.gender},
                                      .comparison = latin::Comparison{g.comparison},
                                      .numeralSort = latin::NumeralSort{g.numeral_sort},
                                      .tense = latin::Tense{g.tense},
                                      .voice = latin::Voice{g.voice},
                                      .mood = latin::Mood{g.mood},
                                      .person = {g.person}},
                      described.data(), described.size());
      Line l;
      l.kind = Kind::Inflection;
      l.stem = form.orth ? form.orth : "";
      l.ending = form.ending ? form.ending : "";
      l.pos = m.pos ? m.pos : "";
      l.qual = qualifier(described.data());
      l.labels = inflectionLabels(form.inflection_age, form.inflection_frequency);
      l.deponent = name<latin::Part>(m.entry.part) == "V" && name<latin::VerbKind>(m.entry.kind) == "DEP";
      l.text = l.stem + (l.ending.empty() ? "" : "." + l.ending) + ' ' + l.qual;
      e.readings.push_back(std::move(l));
    }
    entries.push_back(std::move(e));
  }
  // INFO: New addons start where they differ from the previous entry. A prefix inside a suffix prints again
  //  with each suffix record (Apply_Suffix, appenninum).
  const auto newAddons = [&](const std::size_t j) {
    const auto& addons = entries[j].addons;
    const auto kind = [&](const std::size_t a) {
      return std::string_view(addons[a].first).substr(addons[a].first.find(' ') + 1);
    };
    std::vector<std::size_t> out;
    for (std::size_t a = 0; a < addons.size(); ++a)
      if (j == 0 || a >= entries[j - 1].addonIds.size() || entries[j - 1].addonIds[a] != entries[j].addonIds[a] ||
          (a + 1 < addons.size() && kind(a) == "PREFIX" && kind(a + 1) == "SUFFIX" &&
           (a + 1 >= entries[j - 1].addonIds.size() || entries[j - 1].addonIds[a + 1] != entries[j].addonIds[a + 1])))
        out.push_back(a);
    return out;
  };
  // INFO: WORDS prints an addon as an entry of its own with a null record: the readings after it are printed, its
  //  dictionary form is "" so the next form line is printed, and a meaning before it is never merged across it.
  //  Tricks.Syncope: each rule's marker, a pseudo-entry of its own, and its explanation, printed once a word
  //  (Used_Meanings).
  static constexpr std::array<std::array<std::string_view, 2>, 6> syncopeRules{{
      {"", ""},
      {"Syncope  ii => ivi", "Syncopated perfect ivi can drop 'v' without contracting vowel"},
      {"Syncope   s => vis", "Syncopated perfect often drops the 'v' and contracts vowel"},
      {"Syncope   r => v.r", "Syncopated perfect often drops the 'v' and contracts vowel"},
      {"Syncope  ier=>iver", "Syncopated perfect often drops the 'v' and contracts vowel"},
      {"Syncope s/x => +is", "Syncopated perfect sometimes drops the 'is' after 's' or 'x'"},
  }};
  // INFO: Route lines repeat when the rule changes (sesti has Try_Slury's syncope and then Pass' own).
  const auto newRoute = [&](const std::size_t j) {
    return entries[j].route != kRouteDirect && (j == 0 || entries[j - 1].route != entries[j].route ||
                                                entries[j - 1].first->spelling_rule != entries[j].first->spelling_rule);
  };
  // INFO: Fix_Adverb gives each adjective run its own SUFFIX record (pontice: the c run's, then the ic run's).
  const auto adverbBlock = [&](const std::size_t j) {
    return entries[j].route == kRouteAdverb && (newRoute(j) || entries[j - 1].addonIds != entries[j].addonIds);
  };
  bool explained = false, adverbExplained = false, spellingExplained = false;
  const std::vector<Line>* previousReadings = nullptr;
  std::string previousForm;
  // INFO: WORDS prints the first entry's form, including an addon's or route's.
  bool first = true;
  for (std::size_t j = 0; j < entries.size(); ++j) {
    const auto& e = entries[j];
    const auto& m = *e.first;
    // INFO: Enclitic puts its tackon before the entry and its route lines.
    auto starting = newAddons(j);
    const Entry* carrier = &e;
    // INFO: Fix_Adverb copies the addon records right in front of its adjective run, once, before its SUFFIX record
    //  (WORDS 0b32fb9, Ada bug #4): varieque has que there; dieque has nouns between que and the adjective.
    if (adverbBlock(j)) {
      const auto adjective = [&](const std::size_t k) {
        return entries[k].route == kRouteDirect &&
               std::string_view(entries[k].first->pos ? entries[k].first->pos : "") == "ADJ";
      };
      std::size_t run = j;
      while (run > 0 && !(adjective(run - 1) && entries[run - 1].first->entry.id == m.entry.id))
        --run;
      if (run > 0) {
        --run;
        while (run > 0 && adjective(run - 1) && entries[run - 1].addonIds == entries[run].addonIds)
          --run;
        starting = newAddons(run), carrier = &entries[run];
      }
    }
    if (!starting.empty())
      previousReadings = nullptr, previousForm.clear(), first = false;
    const bool ruleStarts = m.spelling_rule != 0 && (j == 0 || entries[j - 1].first->spelling_rule != m.spelling_rule);
    // INFO: Enclitic takes que, ne, ve, est (ADDONS.LAT's first four tackons) off before a trick reads the rest;
    //  Try_Tricks reads the whole word before Try_Tackons, so any other tackon prints after the rule's lines (hocine,
    //  hisdem).
    const auto enclitic = [&](const std::size_t a) {
      const auto& spelled = carrier->addons[a].first;
      const auto stem = spelled.substr(0, spelled.find(' '));
      return spelled.ends_with(" TACKON") &&
             (stem == "que" || stem == "ne" || stem == "ve" || stem == "ue" || stem == "est");
    };
    const auto addonLines = [&](const bool afterRule) {
      for (const auto a : starting) {
        if (afterRule != (ruleStarts && !enclitic(a)))
          continue;
        const auto& [spelled, meaning] = carrier->addons[a];
        Line l;
        l.kind = Kind::Addon;
        l.stem = spelled.substr(0, spelled.find(' '));
        l.qual = spelled.substr(spelled.find(' ') + 1);
        l.text = spelled;
        out.push_back(std::move(l));
        out.push_back(meaningLine(meaning));
      }
    };
    addonLines(false);
    // INFO: Try_Slury's Flip, Flip_Flop and Slur: the marker, cut at Max_Stem_Size (18), and the explanation (Xxx),
    //  which names the table's pair in its own order even when the word began with the second; before the rewritten
    //  word's own route lines (adcelerasse: Slur, then Syncope).
    if (ruleStarts) {
      const auto rule = whitaker::spellingRule(m.spelling_rule);
      const std::string one = rule.first ? rule.first : "?", two = rule.second ? rule.second : "?";
      std::string marker = rule.reversed ? "Word mod " + two + "/" + one : "Word mod " + one + "/" + two, why;
      switch (rule.kind) {
        case latin::SpellingKind::SLUR:
          marker = rule.reversed ? "Slur " + two + "~/" + one : "Slur " + one + "/" + two + "~";
          why = rule.reversed ? "An initial '" + two + "~' may be rendered by " + one
                              : "An initial '" + one + "' may be rendered by " + two + "~";
          break;
        case latin::SpellingKind::SLURY_SWAP:
          why = "An initial '" + one + "' may be rendered by '" + two + "'";
          break;
        case latin::SpellingKind::REPLACED:
          why = "An initial '" + one + "' may have replaced usual '" + two + "'";
          break;
        case latin::SpellingKind::SWAP:
          why = rule.reversed ? "An initial '" + two + "' may be rendered by '" + one + "'"
                              : "An initial '" + one + "' may be rendered by '" + two + "'";
          break;
        case latin::SpellingKind::INTERNAL:
          why = "An internal '" + one + "' might be rendered by '" + two + "'";
          break;
        case latin::SpellingKind::EO_IIS:
          marker = "Word mod is => iis";
          why = "Some forms of eo stem 'i' grates with an 'is .. .' ending, so 'is' -> 'iis' ";
          break;
        case latin::SpellingKind::TERMINAL_IIS:
          marker = "Word mod iis -> is";
          why = "A Terminal 'iis' on ADJ 1 1 DAT/ABL P might drop 'i'";
          break;
        case latin::SpellingKind::DOUBLED:
          marker = "Word mod " + one + " -> " + two;
          why = "A doubled consonant may be rendered by just the single  MEDIEVAL";
          break;
        default:
          break;
      }
      if (marker.size() > 18)
        marker.resize(18);
      Line l;
      l.kind = Kind::Route, l.text = collapsed(marker);
      out.push_back(l);
      if (!spellingExplained)
        l.text = collapsed(why), out.push_back(l), spellingExplained = true;
      previousReadings = nullptr, previousForm.clear(), first = false;
    }
    addonLines(true);
    if (adverbBlock(j)) {
      // INFO: Fix_Adverb: its SUFFIX record, always e (a superlative's ending me is the reading's), an entry of its
      //  own, and its caption by degree, printed once a word (Used_Meanings).
      const bool super = e.firstForm && name<latin::Comparison>(e.firstForm->grammar.comparison) == "SUPER";
      Line l;
      l.kind = Kind::Addon, l.stem = "e", l.qual = "SUFFIX", l.text = "e SUFFIX";
      out.push_back(l);
      if (!adverbExplained)
        out.push_back(meaningLine(super ? "-estly; -estily; most -ly, very -ly  Converting ADJ to ADV"
                                        : "-ly; -ily;  Converting ADJ to ADV")),
            adverbExplained = true;
      previousReadings = nullptr, previousForm.clear(), first = false;
    } else if (newRoute(j) && e.route != kRouteWordMod && e.route != kRouteSlur) {
      // NOTE: A route the table does not know prints as itself, and differs.
      const auto unknown = std::format("route {}", e.route);
      auto [marker, why] =
          e.route < syncopeRules.size() ? syncopeRules[e.route] : std::array<std::string_view, 2>{unknown, unknown};
      // INFO: The syncope explanation is one slot (Xp.Yyy_Meaning), printed at the end: after Try_Slury, Pass' own
      //  syncope on the word refills it, or leaves it empty (conmendare: the r => v.r inside con/com unexplained).
      const auto kind = whitaker::spellingRule(m.spelling_rule).kind;
      if (kind == latin::SpellingKind::SLUR || kind == latin::SpellingKind::SLURY_SWAP) {
        const auto own = std::ranges::find_if(entries, [](const Entry& x) {
          return x.route >= kRouteSyncopeII && x.route <= kRouteSyncopeIs && x.first->spelling_rule == 0;
        });
        why = own == entries.end() ? std::string_view{} : syncopeRules[own->route][1];
      }
      Line l;
      l.kind = Kind::Route, l.text = collapsed(marker);
      out.push_back(l);
      if (!explained && !why.empty())
        l.text = collapsed(why), out.push_back(l), explained = true;
      previousReadings = nullptr, previousForm.clear(), first = false;
    }
    // INFO: WORDS skips readings matching the previous entry, including labels, folding case, j and v.
    const auto same = [](const std::vector<Line>& a, const std::vector<Line>& b) {
      return std::ranges::equal(a, b, [](const Line& x, const Line& y) {
        return tester::folded(x.stem) == tester::folded(y.stem) &&
               tester::folded(x.ending) == tester::folded(y.ending) && x.qual == y.qual && x.labels == y.labels &&
               x.pos == y.pos;
      });
    };
    if (!previousReadings || !same(e.readings, *previousReadings))
      std::ranges::copy(e.readings, std::back_inserter(out));
    previousReadings = &e.readings;
    const bool formChanged = first || e.form != previousForm;
    first = false;
    previousForm = e.roman ? "" : e.form;
    if (!e.roman) {
      Line l;
      l.kind = Kind::Form;
      l.knownParts = e.record != nullptr;
      const auto gap = e.form.find("  ");
      l.parts = collapsed(std::string_view(e.form).substr(0, gap));
      l.unique = m.entry.id >= std::to_underlying(whitaker::kFirstUnique);
      l.ref = l.unique ? std::format("UNI>({})", m.entry.id - std::to_underlying(whitaker::kFirstUnique) + 1)
                       : std::format("GEN>({})", m.entry.id + 1);
      l.qual = libraryForm(m.entry, e.readings.empty() ? std::string() : e.readings.front().qual);
      l.codes = name<latin::Age>(m.entry.age) + name<latin::Area>(m.entry.area) +
                name<latin::Geography>(m.entry.geography) + name<latin::Frequency>(m.entry.frequency) +
                name<latin::Source>(m.entry.source);
      l.labels = dictionaryLabels(m.entry);
      l.text = std::format("{} {} [{}] {} {}", l.parts, l.qual, l.codes, l.ref, l.labels);
      // INFO: WORDS skips a form line whose Dictionary_Form is the previous one's, codes not compared (aliqua:
      //  GEN>(2600) XXXAO under GEN>(2599) XXXCO). The skipped entry has to agree with its own WORDS record, or its
      //  line is printed and differs (abscisis).
      const bool agrees = e.record && l.codes == e.record->codes &&
                          l.qual == (gap == std::string::npos ? "" : collapsed(std::string_view(e.form).substr(gap)));
      if (formChanged || !agrees)
        out.push_back(std::move(l));
    }
    // INFO: The meaning, unless the next entry has the same one; WORDS compares the raw meanings, bars and all, so by
    //  the records.
    //  The library's meaning has to be the one printed for both, or it is printed and differs (aberant).
    const auto raw = [&](const Entry& x) {
      return x.record ? x.record->meaning : std::string(x.first->meaning ? x.first->meaning : "");
    };
    const auto library = [](const Entry& x) {
      return std::pair(std::string_view(x.first->meaning ? x.first->meaning : ""), x.first->entry.numeral_value);
    };
    if (e.roman || j + 1 == entries.size() || entries[j + 1].roman || !newAddons(j + 1).empty() || newRoute(j + 1) ||
        adverbBlock(j + 1) || raw(e) != raw(entries[j + 1]) || library(e) != library(entries[j + 1])) {
      // INFO: Put_Meaning_Line constructs a numeral's meaning from its value, or leaves it blank for some sorts.
      if (!e.roman && name<latin::Part>(m.entry.part) == "NUM" && m.entry.numeral_value > 0) {
        auto l = meaningLine(
            constructedMeaning(m.entry.numeral_value, name<latin::Part>(e.firstForm ? e.firstForm->grammar.part : 0),
                               name<latin::NumeralSort>(e.firstForm ? e.firstForm->grammar.numeral_sort : 0)));
        l.numeral = true;
        if (!l.qual.empty())
          out.push_back(std::move(l));
      } else {
        // NOTE: The library gives a Roman numeral's value apart from its meaning; WORDS prints " 23  as a ROMAN
        //  NUMERAL;" (Integer'Image's blank, then two).
        const std::string meaning = m.meaning ? m.meaning : "";
        out.push_back(meaningLine(e.roman ? std::format(" {}  {}", m.entry.numeral_value, meaning) : meaning));
      }
    }
  }
  if (trimmed) {
    Line l;
    l.kind = Kind::Trimmed, l.text = "*";
    out.push_back(l);
  }
  return out;
}

} // namespace lines
