#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <format>
#include <functional>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <meta>
#include <memory>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include <whitaker.hpp>

// INFO: The adapter's own copies of a reading, filled from Result::decode, which --probe changes field by field.
struct WhitakerAddonStep {
  uint16_t id;
  uint8_t kind;
};
struct WhitakerGrammar {
  uint8_t part;
  uint8_t which;
  uint8_t variant;
  uint8_t case_of;
  uint8_t number;
  uint8_t gender;
  uint8_t comparison;
  uint8_t numeral_sort;
  uint8_t tense;
  uint8_t voice;
  uint8_t mood;
  uint8_t person;
};
struct WhitakerEntry {
  uint8_t part;
  uint8_t which;
  uint8_t variant;
  uint8_t gender;
  uint8_t kind;
  uint8_t comparison;
  uint8_t numeral_sort;
  uint8_t area;
  uint8_t geography;
  uint8_t source;
  uint16_t numeral_value;
  uint8_t age;
  uint8_t frequency;
  uint16_t id;
};
struct WhitakerForm {
  const char* orth;
  const char* ending;
  WhitakerGrammar grammar;
  uint8_t inflection_age;
  uint8_t inflection_frequency;
};
struct WhitakerRecord {
  WhitakerEntry entry;
  const char* meaning;
  const char* pos;
  const WhitakerAddonStep* addons;
  uint8_t addon_count;
  uint8_t route;
  uint8_t spelling_rule;
  size_t first_form;
  size_t form_count;
};

#include "ada_parse.hpp"
#include "dictionary.hpp"

namespace lines {

namespace fs = std::filesystem;
using namespace std::string_view_literals;

enum class Rule : std::uint8_t {
  Ending,
  InflectionLabel,
  PrincipalParts,
  DictionaryRef,
  Route,
  Trimmed,
  SpellingFold,
  MeaningBar,
  MeaningWidth,
  PersonZero,
  DeponentVoice,
  Count,
};
struct RuleInfo {
  Rule rule;
  std::string_view name, kind, why;
};
inline constexpr std::array<RuleInfo, std::to_underlying(Rule::Count)> kRules{{
    {Rule::Ending, "ending", "gap", "the inflection after the stem"},
    {Rule::InflectionLabel, "inflection-label", "gap", "an inflection's age/frequency (Early, uncommon)"},
    {Rule::PrincipalParts, "principal-parts", "gap", "the dictionary form before the part"},
    {Rule::DictionaryRef, "dictionary-ref", "gap", "the entry's file and number (GEN>(2810))"},
    {Rule::Route, "route", "gap", "the trick or route WORDS took (Syncope, Two words)"},
    {Rule::Trimmed, "trimmed", "gap", "that rarer readings were left out (*)"},
    {Rule::SpellingFold, "spelling-fold", "reading", "WORDS matches j as i and v as u, in any case"},
    {Rule::MeaningBar, "meaning-bar", "reading", "WORDS' Trim_Bar strips a continuation's leading |"},
    {Rule::MeaningWidth, "meaning-width", "reading", "WORDS shows the first 79 characters of a meaning"},
    {Rule::PersonZero, "person-zero", "reading",
     "WORDS prints a verb's person 0 (INF 0 X), whitaker_describe leaves it out"},
    {Rule::DeponentVoice, "deponent-voice", "reading",
     "WORDS blanks a deponent's voice field, whatever it holds: not shown, not compared"},
}};
static_assert(std::ranges::all_of(std::views::iota(0uz, kRules.size()),
                                  [](const std::size_t i) { return std::to_underlying(kRules[i].rule) == i; }));
using Rules = std::set<Rule>;

enum class Kind : std::uint8_t { Inflection, Form, Meaning, Addon, Unknown, Route, Trimmed };
inline constexpr std::array kKindNames{"inflection"sv, "form"sv,  "meaning"sv, "addon"sv,
                                       "unknown"sv,    "route"sv, "trimmed"sv};

struct Line {
  Kind kind{};
  std::string text;
  // INFO: Inflection: stem, ending, qual, labels. Addon: stem (spelling), qual (kind). Form: parts, qual (form), codes,
  //  ref, labels. Meaning: qual (the text).
  std::string stem, ending, qual, parts, codes, ref, labels;
  bool knownEnding = true, knownParts = true, knownRef = true;
  bool unique = false;
  bool deponent = false;
  std::string pos;
  std::string printedForm;
  bool numeral = false;
  std::string raw;
};

inline std::string collapsed(const std::string_view s) {
  std::string out;
  for (const auto w : tester::wordsOf(s))
    out += (out.empty() ? "" : " ") + std::string(w);
  return out;
}

// INFO: --probe changes every record and form, or the one at this index.
inline constexpr std::size_t kAll = SIZE_MAX;
struct Mutation {
  std::function<void(WhitakerRecord&)> record;
  std::function<void(WhitakerForm&)> form;
};

enum class Status : std::uint8_t { Equal, Differs, Gap };
struct Field {
  std::string_view name;
  Status status{};
  std::string ada, library;
};

enum class Verdict : std::uint8_t { Maps, Gaps, Differs };

struct Pairing {
  const Line* ada = nullptr;
  const Line* library = nullptr;
  Rules rules;
  std::vector<Field> fields;
  bool differs = false;
};

struct Spelling {
  std::string word, status;
  std::uint64_t formsRebuilt = 0;
  std::vector<std::string> formsDiffering;
  std::vector<Line> ada, library;
  std::vector<Pairing> pairs;
  Verdict verdict = Verdict::Maps;
};

struct Seen {
  std::string response;
  std::uint64_t tokens = 0;
};

std::vector<Line> adaLines(std::string_view response);
const Dictionary& wordsDictionary();
std::vector<Line> libraryLines(const std::string& word, std::string& status, const Mutation* mutate = nullptr,
                               std::size_t only = kAll);
std::array<std::string, 3> qualifierFields(std::string_view qual);
Spelling compare(const std::string& word, std::string_view response, const Mutation* mutate = nullptr,
                 std::size_t only = kAll);
std::string pattern(const Pairing& p);
std::string readFile(const fs::path& path);
// INFO: Throws if the file cannot be opened or fully written.
void write(const fs::path& path, const std::function<void(std::ostream&)>& fill);
std::map<std::string, Seen> load(const fs::path& captures, std::vector<std::string>& names, std::uint64_t& bookTokens,
                                 bool skipIncomplete = false);
int importAnswers(const fs::path& captures, const fs::path& out);
int show(const fs::path& captures, const std::vector<std::string>& words);
int probe(const fs::path& captures, std::vector<std::string> names);
int run(const fs::path& captures, const fs::path& out, std::vector<std::string> names);

} // namespace lines
