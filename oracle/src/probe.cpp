#include "lines.hpp"

#include <meta>

namespace lines {

namespace {

struct Mapping {
  std::string_view field, ada, note;
};
constexpr std::array kMappings{
    Mapping{"form.orth", "stem", ""},
    Mapping{"form.ending", "ending", ""},
    Mapping{"record.meaning", "meaning", ""},
    Mapping{"record.pos", "part", "the reading's part by name"},
    Mapping{"form.grammar.part", "part", ""},
    Mapping{"form.grammar.which", "declension/conjugation", ""},
    Mapping{"form.grammar.variant", "declension/conjugation", ""},
    Mapping{"form.grammar.case_of", "grammar", ""},
    Mapping{"form.grammar.number", "grammar", ""},
    Mapping{"form.grammar.gender", "grammar", ""},
    Mapping{"form.grammar.comparison", "grammar", ""},
    Mapping{"form.grammar.numeral_sort", "grammar", ""},
    Mapping{"form.grammar.tense", "grammar", ""},
    Mapping{"form.grammar.voice", "grammar", "not a deponent's: WORDS blanks it"},
    Mapping{"form.grammar.mood", "grammar", ""},
    Mapping{"form.grammar.person", "grammar", ""},
    Mapping{"record.entry.part", "form", ""},
    Mapping{"record.entry.which", "form", "a noun's declension, a verb's conjugation; WORDS shows no other"},
    Mapping{"record.entry.variant", "form", "only where it decides the declension or conjugation shown"},
    Mapping{"record.entry.gender", "form", "a noun's"},
    Mapping{"record.entry.kind", "form", "a verb's kind; WORDS shows no noun, pronoun or packon kind"},
    Mapping{"record.entry.comparison", "", "WORDS shows it only through the principal parts"},
    Mapping{"record.entry.numeral_sort", "", "WORDS shows the reading's (grammar.numeral_sort), not the entry's"},
    Mapping{"record.entry.area", "area", ""},
    Mapping{"record.entry.geography", "geography", ""},
    Mapping{"record.entry.source", "source", ""},
    Mapping{"record.entry.numeral_value", "numeral meaning", "the meaning WORDS makes up for a numeral"},
    Mapping{"record.entry.age", "age", "and the entry labels"},
    Mapping{"record.entry.frequency", "frequency", "and the entry labels"},
    Mapping{"record.entry.id", "dictionary ref",
            "GEN>(id + 1), UNI>(...); and the principal parts, from WORDS' record by id"},
    Mapping{"record.addons.id", "addon", "spelling and meaning"},
    Mapping{"record.addons.kind", "addon kind", ""},
    Mapping{"record.addon_count", "addon", ""},
    Mapping{"record.route", "route", "WORDS' marker and explanation lines of a syncope rule or a spelling rule"},
    Mapping{"record.spelling_rule", "route", "the Word mod or Slur rule: WORDS' marker and explanation"},
    Mapping{"form.inflection_age", "inflection labels", "shown when not X"},
    Mapping{"form.inflection_frequency", "inflection labels", "shown when C or rarer"},
    Mapping{"record.first_form", "inflection", "which forms the record holds"},
    Mapping{"record.form_count", "inflection", "which forms the record holds"},
};

consteval void fieldPaths(std::meta::info type, std::string_view prefix, std::vector<std::string>& out) {
  for (auto m : std::meta::nonstatic_data_members_of(type, std::meta::access_context::unchecked())) {
    std::string path(prefix);
    if (!path.empty())
      path += '.';
    path += std::meta::identifier_of(m);
    auto t = std::meta::remove_all_extents(std::meta::type_of(m));
    if (std::meta::is_pointer_type(t) && t != ^^const char*)
      t = std::meta::remove_const(std::meta::remove_pointer(t));
    if (std::meta::is_class_type(t))
      fieldPaths(t, path, out);
    else
      out.push_back(path);
  }
}

consteval std::string unmappedField() {
  std::vector<std::string> paths;
  fieldPaths(^^WhitakerRecord, "record", paths);
  fieldPaths(^^WhitakerForm, "form", paths);
  for (const auto& p : paths)
    if (!std::ranges::contains(kMappings, std::string_view(p), &Mapping::field))
      return "not mapped: " + p;
  for (const auto& m : kMappings)
    if (!std::ranges::contains(paths, m.field))
      return "no such field: " + std::string(m.field);
  return "";
}
constexpr std::string_view kUnmapped = std::define_static_string(unmappedField());
static_assert(kUnmapped.empty(), kUnmapped);

template <typename T>
inline constexpr auto kMembersOf =
    std::define_static_array(std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()));

template <typename Root> Mutation changing(std::function<void(Root&)> change) {
  Mutation m;
  if constexpr (std::is_same_v<Root, WhitakerRecord>)
    m.record = std::move(change);
  else
    m.form = std::move(change);
  return m;
}

template <typename Root, typename T>
void mutations(std::string_view prefix, std::function<T&(Root&)> get,
               std::vector<std::pair<std::string, Mutation>>& out) {
  template for (constexpr auto m : kMembersOf<T>) {
    using M = typename[:std::meta::type_of(m):];
    const auto path = prefix.empty() ? std::string(std::meta::identifier_of(m))
                                     : std::format("{}.{}", prefix, std::meta::identifier_of(m));
    if constexpr (std::is_array_v<M>) {
      using E = std::remove_extent_t<M>;
      mutations<Root, E>(path, std::function<E&(Root&)>([get](Root& w) -> E& { return get(w).[:m:][0]; }), out);
    } else if constexpr (std::is_pointer_v<M> && !std::is_same_v<M, const char*>) {
      // NOTE: The add-on steps: libraryLines hands the probe its own copy to change.
      using E = std::remove_const_t<std::remove_pointer_t<M>>;
      mutations<Root, E>(
          path, std::function<E&(Root&)>([get](Root& w) -> E& { return const_cast<E&>(get(w).[:m:][0]); }), out);
    } else if constexpr (std::is_class_v<M>) {
      mutations<Root, M>(path, std::function<M&(Root&)>([get](Root& w) -> M& { return get(w).[:m:]; }), out);
    } else if constexpr (std::is_same_v<M, const char*>) {
      out.emplace_back(path, changing<Root>([get](Root& w) { get(w).[:m:] = "zzz"; }));
    } else {
      out.emplace_back(path, changing<Root>([get](Root& w) { get(w).[:m:] = static_cast<M>(get(w).[:m:] + 1); }));
    }
  }
}

} // namespace

int probe(const fs::path& captures, std::vector<std::string> names) {
  std::uint64_t bookTokens = 0;
  const auto spellings = load(captures, names, bookTokens);
  const auto differing = [](const Spelling& s) {
    std::set<std::string> out;
    for (const auto& p : s.pairs) {
      for (const auto& f : p.fields)
        if (f.status == Status::Differs)
          out.insert(std::string(f.name));
      if (p.differs && (!p.ada || !p.library))
        out.insert(std::format("{} {}", p.ada ? "Ada-only" : "library-only",
                               kKindNames[std::to_underlying((p.ada ? p.ada : p.library)->kind)]));
    }
    return out;
  };
  std::map<std::string, std::set<std::string>> before;
  for (const auto& [word, seen] : spellings)
    before[word] = differing(compare(word, seen.response));

  std::vector<std::pair<std::string, Mutation>> all;
  mutations<WhitakerRecord, WhitakerRecord>(
      "record", std::function<WhitakerRecord&(WhitakerRecord&)>([](WhitakerRecord& r) -> WhitakerRecord& { return r; }),
      all);
  mutations<WhitakerForm, WhitakerForm>(
      "form", std::function<WhitakerForm&(WhitakerForm&)>([](WhitakerForm& f) -> WhitakerForm& { return f; }), all);
  // NOTE: Each record and form alone too: WORDS prints a line shared by neighbouring entries once, so a change to
  //  one of them must still be noticed.
  std::map<std::string, std::array<std::size_t, 2>> counts;
  for (const auto& [word, seen] : spellings) {
    auto& [records, forms] = counts[word];
    const Mutation counting{[&](WhitakerRecord&) { ++records; }, [&](WhitakerForm&) { ++forms; }};
    compare(word, seen.response, &counting);
  }
  struct Row {
    bool alone;
    std::string path;
    const Mutation* mutate;
    std::uint64_t tried = 0, noticed = 0;
    std::map<std::string, std::uint64_t> where;
  };
  std::vector<Row> rows;
  for (const bool alone : {false, true})
    for (const auto& [path, mutate] : all)
      rows.push_back({alone, path, &mutate});
  std::atomic<std::size_t> next = 0;
  {
    std::vector<std::jthread> workers;
    for (unsigned t = 0; t < std::max(1u, std::thread::hardware_concurrency()); ++t)
      workers.emplace_back([&] {
        for (std::size_t r; (r = next++) < rows.size();) {
          auto& row = rows[r];
          for (const auto& [word, seen] : spellings) {
            const auto n = !row.alone ? 1 : counts.at(word)[row.path.starts_with("form.")];
            for (std::size_t k = 0; k < n; ++k) {
              const auto& was = before.at(word);
              bool any = false;
              // NOTE: One entry.id changed into another record's id: the same-id check stops the run, so noticed.
              try {
                for (const auto& f : differing(compare(word, seen.response, row.mutate, row.alone ? k : kAll)))
                  if (!was.contains(f))
                    ++row.where[f], any = true;
              } catch (const std::logic_error&) {
                ++row.where["same-id check"], any = true;
              }
              ++row.tried, row.noticed += any;
            }
          }
        }
      });
  }
  bool ok = true;
  for (std::size_t r = 0; r < rows.size(); ++r) {
    const auto& [alone, path, mutate, tried, noticed, where] = rows[r];
    if (r == 0 || alone != rows[r - 1].alone) {
      std::cout << std::format(
          "{}{} spellings; {} library fields, each changed in {}\n\n", r ? "\n" : "", spellings.size(), all.size(),
          alone ? "one record or form at a time (an entry field: every record of its id)" : "every record and form");
      std::cout << std::format("{:<22} {:<24} {:>15}  {}\n", "library field", "mapped to", "noticed/tried",
                               alone ? "where (changes)" : "where (spellings)");
    }
    const auto* row = std::ranges::find(kMappings, std::string_view(path), &Mapping::field);
    std::vector<std::pair<std::string, std::uint64_t>> sorted(where.begin(), where.end());
    std::ranges::stable_sort(sorted, std::greater{}, &std::pair<std::string, std::uint64_t>::second);
    std::string top;
    for (std::size_t i = 0; i < sorted.size() && i < 4; ++i)
      top += std::format("{}{} ({})", i ? ", " : "", sorted[i].first, sorted[i].second);
    const std::string mapped(row->ada);
    const bool expected = row->ada.empty() ? noticed == 0
                                           : where.contains(mapped) || where.contains("library-only " + mapped) ||
                                                 where.contains("Ada-only " + mapped);
    ok = ok && expected;
    std::cout << std::format("{:<22} {:<24} {:>15}  {}{}\n", path, row->ada.empty() ? "(not shown)" : row->ada,
                             std::format("{}/{}", noticed, tried), top, expected ? "" : "   <-- NOT AS MAPPED");
  }
  std::cout << (ok ? "\nevery field as mapped\n" : "\nsome fields not as mapped\n");
  return ok ? 0 : 1;
}

} // namespace lines
