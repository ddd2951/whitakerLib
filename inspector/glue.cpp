#include <latin_generated.hpp>
#include <whitaker.hpp>

#include "trace.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#else
#include <chrono>
#define EMSCRIPTEN_KEEPALIVE
[[nodiscard]] static double emscripten_get_now() {
  return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
#endif

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <type_traits>
#include <vector>

namespace {

namespace trace = whitaker::trace;

template <typename T> [[nodiscard]] constexpr std::uint8_t byte(T value) {
  if constexpr (std::is_enum_v<T>)
    return std::to_underlying(value);
  else if constexpr (requires { value.value; })
    return value.value;
  else
    return static_cast<std::uint8_t>(value);
}
template <typename E, typename T> [[nodiscard]] const char* nameOf(T value) {
  return byte(value) ? latin::name(E{byte(value)}) : nullptr;
}
[[nodiscard]] const char* spokenName(std::string_view type, unsigned value) {
  if (type == "status") {
    switch (whitaker::Status(value)) {
      case whitaker::Status::ok:
        return "ok";
      case whitaker::Status::invalidArgument:
        return "invalid argument";
      case whitaker::Status::inputTooLong:
        return "input too long";
      case whitaker::Status::outOfMemory:
        return "out of memory";
    }
    return nullptr;
  }
  switch (latin::AddonKind(value)) {
    case latin::AddonKind::tickon:
      return "tickon";
    case latin::AddonKind::prefix:
      return "prefix";
    case latin::AddonKind::suffix:
      return "suffix";
    case latin::AddonKind::tackon:
      return "tackon";
    case latin::AddonKind::packon:
      return "packon";
  }
  return nullptr;
}
[[nodiscard]] const char* kindName(latin::Part part, unsigned kind) {
  if (!kind)
    return nullptr;
  const auto value = static_cast<std::uint8_t>(kind);
  switch (part) {
    case latin::Part::noun:
      return latin::name(latin::NounKind{value});
    case latin::Part::pronoun:
      return latin::name(latin::PronounKind{value});
    case latin::Part::packon:
      return latin::name(latin::PackonKind{value});
    case latin::Part::verb:
      return latin::name(latin::VerbKind{value});
    default:
      return nullptr;
  }
}
enum class Field : unsigned char {
  part,
  caseOf,
  number,
  gender,
  comparison,
  numeralSort,
  tense,
  voice,
  mood,
};

struct Words {
  Field field;
  std::uint8_t value;
  std::string_view full;
  std::string_view brief;
};

constexpr Words kWords[] = {
    {Field::part, std::to_underlying(latin::Part::noun), "noun", ""},
    {Field::part, std::to_underlying(latin::Part::pronoun), "pronoun", ""},
    {Field::part, std::to_underlying(latin::Part::verb), "verb", ""},
    {Field::part, std::to_underlying(latin::Part::adjective), "adjective", ""},
    {Field::part, std::to_underlying(latin::Part::adverb), "adverb", ""},
    {Field::part, std::to_underlying(latin::Part::preposition), "preposition", ""},
    {Field::part, std::to_underlying(latin::Part::numeral), "numeral", ""},
    {Field::part, std::to_underlying(latin::Part::conjunction), "conjunction", ""},
    {Field::part, std::to_underlying(latin::Part::interjection), "interjection", ""},
    {Field::part, std::to_underlying(latin::Part::packon), "pronoun", ""},
    {Field::part, std::to_underlying(latin::Part::supine), "supine", ""},
    {Field::part, std::to_underlying(latin::Part::participle), "participle", ""},
    {Field::caseOf, std::to_underlying(latin::Case::nominative), "nominative", "nom"},
    {Field::caseOf, std::to_underlying(latin::Case::vocative), "vocative", "voc"},
    {Field::caseOf, std::to_underlying(latin::Case::genitive), "genitive", "gen"},
    {Field::caseOf, std::to_underlying(latin::Case::locative), "locative", "loc"},
    {Field::caseOf, std::to_underlying(latin::Case::dative), "dative", "dat"},
    {Field::caseOf, std::to_underlying(latin::Case::ablative), "ablative", "abl"},
    {Field::caseOf, std::to_underlying(latin::Case::accusative), "accusative", "acc"},
    {Field::number, std::to_underlying(latin::Number::singular), "singular", "sg"},
    {Field::number, std::to_underlying(latin::Number::plural), "plural", "pl"},
    {Field::gender, std::to_underlying(latin::Gender::masculine), "masculine", "m"},
    {Field::gender, std::to_underlying(latin::Gender::feminine), "feminine", "f"},
    {Field::gender, std::to_underlying(latin::Gender::neuter), "neuter", "n"},
    {Field::gender, std::to_underlying(latin::Gender::common), "common", "c"},
    {Field::comparison, std::to_underlying(latin::Comparison::comparative), "comparative", "comp"},
    {Field::comparison, std::to_underlying(latin::Comparison::superlative), "superlative", "superl"},
    {Field::tense, std::to_underlying(latin::Tense::present), "present", "pres"},
    {Field::tense, std::to_underlying(latin::Tense::imperfect), "imperfect", "impf"},
    {Field::tense, std::to_underlying(latin::Tense::future), "future", "fut"},
    {Field::tense, std::to_underlying(latin::Tense::perfect), "perfect", "perf"},
    {Field::tense, std::to_underlying(latin::Tense::pluperfect), "pluperfect", "plup"},
    {Field::tense, std::to_underlying(latin::Tense::futurePerfect), "future perfect", "fut perf"},
    {Field::voice, std::to_underlying(latin::Voice::active), "active", "act"},
    {Field::voice, std::to_underlying(latin::Voice::passive), "passive", "pass"},
    {Field::mood, std::to_underlying(latin::Mood::indicative), "indicative", "ind"},
    {Field::mood, std::to_underlying(latin::Mood::subjunctive), "subjunctive", "subj"},
    {Field::mood, std::to_underlying(latin::Mood::imperative), "imperative", "imp"},
    {Field::mood, std::to_underlying(latin::Mood::infinitive), "infinitive", "inf"},
    {Field::mood, std::to_underlying(latin::Mood::participle), "participle", "ptc"},
};

template <typename T> [[nodiscard]] std::string_view inWords(Field field, T value, bool brief = false) {
  for (const Words& w : kWords)
    if (w.field == field && w.value == byte(value))
      return brief ? w.brief : w.full;
  return "";
}

[[nodiscard]] std::string ordinal(unsigned n) {
  if (n < 1 || n > 9)
    return "";
  return std::to_string(n) + (n == 1 ? "st" : n == 2 ? "nd" : n == 3 ? "rd" : "th");
}

void append(std::string& out, std::string_view word) {
  if (word.empty())
    return;
  if (!out.empty())
    out += ' ';
  out += word;
}

[[nodiscard]] std::string formInWords(const latin::Analysis& g, bool brief = false) {
  std::string out;
  if (g.person.value)
    append(out, brief ? std::to_string(g.person.value) : ordinal(g.person.value) + " person");
  else
    append(out, inWords(Field::caseOf, g.caseOf, brief));
  append(out, inWords(Field::number, g.number, brief));
  append(out, inWords(Field::gender, g.gender, brief));
  append(out, inWords(Field::comparison, g.comparison, brief));
  append(out, inWords(Field::tense, g.tense, brief));
  append(out, inWords(Field::voice, g.voice, brief));
  append(out, inWords(Field::mood, g.mood, brief));
  if (g.part == latin::Part::supine)
    append(out, "supine");
  return out;
}

[[nodiscard]] std::string classInWords(const latin::DictionaryWord& word) {
  std::string out{inWords(Field::part, word.part)};
  const std::string nth = ordinal(word.which.value);
  if (nth.empty() || out.empty())
    return out;
  if (word.part == latin::Part::verb)
    out += ", " + nth + " conjugation";
  else if (word.part == latin::Part::adjective && word.which.value == 1)
    out += ", 1st/2nd declension";
  else if (word.part == latin::Part::noun || word.part == latin::Part::adjective || word.part == latin::Part::pronoun)
    out += ", " + nth + " declension";
  return out;
}

[[nodiscard]] std::string_view stemInWords(latin::Part part, std::uint8_t key) {
  static constexpr std::string_view kNoun[] = {"nominative stem", "stem of the other cases", "", ""};
  static constexpr std::string_view kAdjective[] = {"nominative stem", "stem of the other cases", "comparative stem",
                                                    "superlative stem"};
  static constexpr std::string_view kVerb[] = {"present stem", "present stem", "perfect stem", "participle stem"};
  static constexpr std::string_view kNumeral[] = {"cardinal stem", "ordinal stem", "distributive stem",
                                                  "numeral adverb stem"};
  if (key < 1 || key > 4)
    return "";
  switch (latin::Part{part}) {
    case latin::Part::noun:
    case latin::Part::pronoun:
    case latin::Part::packon:
      return kNoun[key - 1];
    case latin::Part::adjective:
      return kAdjective[key - 1];
    case latin::Part::verb:
      return kVerb[key - 1];
    case latin::Part::numeral:
      return kNumeral[key - 1];
    default:
      return "";
  }
}

[[nodiscard]] std::string_view orEmpty(const char* text) { return text ? text : ""; }

[[nodiscard]] std::string joined(std::initializer_list<std::string_view> parts) {
  std::string out;
  for (const std::string_view part : parts)
    if (!part.empty()) {
      if (!out.empty())
        out += " · ";
      out += part;
    }
  return out;
}

[[nodiscard]] std::string count(std::size_t n) {
  std::string digits = std::to_string(n);
  for (std::size_t at = digits.size(); at > 3; at -= 3)
    digits.insert(at - 3, ",");
  return digits;
}

[[nodiscard]] std::string fixed(double value, int decimals) {
  char text[32];
  std::snprintf(text, sizeof text, "%.*f", decimals, value);
  return text;
}

[[nodiscard]] std::string duration(double microseconds) {
  return microseconds < 1000 ? fixed(microseconds, 1) + " µs" : fixed(microseconds / 1000, 2) + " ms";
}

[[nodiscard]] std::string lower(std::string_view text) {
  std::string out{text};
  for (char& c : out)
    if (c >= 'A' && c <= 'Z')
      c = static_cast<char>(c - 'A' + 'a');
  return out;
}

void escape(std::string& out, std::string_view text) {
  for (const char c : text)
    out += c == '&' ? "&amp;" : c == '<' ? "&lt;" : c == '>' ? "&gt;" : c == '"' ? "&quot;" : std::string{c};
}

class Html {
public:
  void clear() { out.clear(); }
  [[nodiscard]] const char* text() const { return out.c_str(); }
  Html& open(std::string_view tag, std::string_view cls = {}, std::string_view attributes = {}) {
    out.append("<").append(tag).append(cls.empty() ? "" : " class=\"").append(cls).append(cls.empty() ? "" : "\"");
    out.append(attributes.empty() ? "" : " ").append(attributes).append(">");
    return *this;
  }
  Html& close(std::string_view tag) {
    out.append("</").append(tag).append(">");
    return *this;
  }
  Html& add(std::string_view text) {
    escape(out, text);
    return *this;
  }
  Html& element(std::string_view tag, std::string_view cls, std::string_view text) {
    return open(tag, cls).add(text).close(tag);
  }

private:
  std::string out;
};

constexpr std::size_t kNoWord = SIZE_MAX;

struct Word {
  std::string spelling;
  whitaker::Status status = whitaker::Status::ok;
  std::size_t index = kNoWord;
  std::size_t firstBlock{}, blockCount{};
  std::vector<trace::Step> steps;
  std::string path;
};

struct Block {
  std::size_t first{}, count{};
};

struct Text {
  whitaker::Result result;
  std::vector<whitaker::ReadingDetail> details;
  std::vector<Block> blocks;
  std::vector<Word> words;
  std::vector<trace::Tags> tags;
  std::vector<std::uint32_t> occurrences;
};

Text loadedText;
Html html;
std::string reply;

struct Group {
  std::uint32_t word;
  std::size_t count;
};

std::size_t selectedFrom = 0;
std::size_t selectedTo = 0;
std::vector<Group> selectedGroups;

[[nodiscard]] std::vector<Group> groupsOf(std::size_t from, std::size_t to) {
  std::vector<Group> groups;
  std::vector<std::uint32_t> slot(loadedText.words.size(), UINT32_MAX);
  for (std::size_t i = from; i <= to && i < loadedText.occurrences.size(); ++i) {
    const std::uint32_t w = loadedText.occurrences[i];
    if (slot[w] == UINT32_MAX) {
      slot[w] = static_cast<std::uint32_t>(groups.size());
      groups.push_back({w, 0});
    }
    ++groups[slot[w]].count;
  }
  return groups;
}

[[nodiscard]] std::span<const Block> recordsOf(const Word& word) {
  return std::span{loadedText.blocks}.subspan(word.firstBlock, word.blockCount);
}
[[nodiscard]] std::size_t readingCount(const Word& word) {
  return word.index == kNoWord ? 0 : whitaker::readingCount(loadedText.result, whitaker::WordId{word.index});
}
[[nodiscard]] bool trimmed(const Word& word) {
  return word.index != kNoWord && whitaker::trimmed(loadedText.result, whitaker::WordId{word.index});
}
[[nodiscard]] const whitaker::ReadingDetail& head(const Block& block) { return loadedText.details[block.first]; }

struct PassRun {
  const char* pass;
  bool ran;
  unsigned added;
};

[[nodiscard]] std::vector<PassRun> passesOf(const Word& word) {
  std::vector<PassRun> out;
  int before = 0;
  for (auto pass = std::to_underlying(trace::Pass::dictionary); pass <= std::to_underlying(trace::Pass::adverbs);
       ++pass) {
    const trace::Step* ran = nullptr;
    for (const trace::Step& step : word.steps)
      if (std::to_underlying(step.pass) == pass)
        ran = &step;
    unsigned added = 0;
    if (ran) {
      added = static_cast<unsigned>(ran->readings > before ? ran->readings - before : 0);
      before = ran->readings;
    }
    out.push_back({trace::name(trace::Pass{pass}), ran != nullptr, added});
  }
  return out;
}

[[nodiscard]] std::string grammarOf(const whitaker::Form& form) {
  char grammar[64];
  latin::describe(form.grammar, grammar, sizeof grammar);
  return grammar;
}

[[nodiscard]] const char* foundBy(std::size_t form) {
  const trace::Tags& tags = loadedText.tags[form];
  return tags.foundBy == trace::kNone ? nullptr : trace::name(trace::Pass{tags.foundBy});
}

// INFO: An identifier as words: wordMod as "word mod".
[[nodiscard]] std::string spoken(const std::string_view name) {
  std::string out{};
  for (const char c : name)
    if (c >= 'A' && c <= 'Z')
      out.append(1, ' ').append(1, static_cast<char>(c - 'A' + 'a'));
    else
      out += c;
  return out;
}

[[nodiscard]] std::string routeOf(const Block& record) {
  const whitaker::ReadingDetail& d = head(record);
  const std::string route = spoken(orEmpty(latin::name(d.route)));
  return d.route == latin::Route::syncope ? route + " " + lower(orEmpty(latin::name(d.form.derivation.syncope)))
                                          : route;
}

[[nodiscard]] std::string spellingLine(const Block& record) {
  if (!head(record).form.spellingRuleId)
    return "";
  const auto rule = whitaker::spellingRule(head(record).form.spellingRuleId);
  const std::string first{orEmpty(rule.first)}, second{orEmpty(rule.second)};
  return "respelled " + (rule.reversed ? second + " → " + first : first + " → " + second);
}

[[nodiscard]] std::string addonsLine(const Block& record) {
  std::string out;
  for (const whitaker::AddonStep& step : std::span{head(record).addons, head(record).addonCount}) {
    if (!out.empty())
      out += ", ";
    out += std::string{orEmpty(spokenName("addon", byte(step.kind)))} + " " +
           std::string{orEmpty(whitaker::addonSpelling(step.id))} + " (" +
           std::string{orEmpty(whitaker::addonMeaning(step.id))} + ")";
  }
  return out;
}

[[nodiscard]] std::string stemsLine(const whitaker::DictionaryWordStems& stems) {
  std::string out;
  for (unsigned i = 0; i < stems.count; ++i)
    if (i == 0 || std::string_view{stems.stems[i].spelling} != stems.stems[i - 1].spelling)
      out += (out.empty() ? "stems " : ", ") + std::string{stems.stems[i].spelling};
  return out;
}

struct Value {
  std::string_view field;
  std::string value;
};

void readingValues(const Block& record, std::size_t f, std::vector<Value>& out) {
  const whitaker::Form& form = loadedText.details[f].form;
  const latin::Analysis& g = form.grammar;
  const std::string grammar = grammarOf(form);
  out.push_back({"part", grammar.substr(0, grammar.find(' '))});
  const auto add = [&](std::string_view field, const char* value) {
    if (value)
      out.push_back({field, value});
  };
  add("case", nameOf<latin::Case>(g.caseOf));
  add("number", nameOf<latin::Number>(g.number));
  add("gender", nameOf<latin::Gender>(g.gender));
  add("tense", nameOf<latin::Tense>(g.tense));
  add("voice", nameOf<latin::Voice>(g.voice));
  add("mood", nameOf<latin::Mood>(g.mood));
  out.push_back({"route", routeOf(record)});
  for (const whitaker::AddonStep& step : std::span{head(record).addons, head(record).addonCount})
    out.push_back({"add-on", std::string{orEmpty(spokenName("addon", byte(step.kind)))} + " " +
                                 std::string{orEmpty(whitaker::addonSpelling(step.id))}});
}

void lookUp(const std::string& spelling) {
  Text& t = loadedText;
  Word word;
  word.spelling = spelling;
  [[maybe_unused]] const std::size_t formsBefore = t.details.size();
  word.status = t.words.empty() ? whitaker::analyze(t.result, spelling) : whitaker::append(t.result, spelling);
  if (word.status == whitaker::Status::ok) {
    word.index = whitaker::wordCount(t.result) - 1;
    word.firstBlock = t.blocks.size();
    const whitaker::WordId added{word.index};
    for (std::size_t r = 0; r < whitaker::readingCount(t.result, added); ++r) {
      const whitaker::ReadingDetail detail = whitaker::detail(t.result, added, r);
      const auto same = [](const whitaker::ReadingDetail& a, const whitaker::ReadingDetail& b) {
        if (a.dictionaryWordId != b.dictionaryWordId || a.form.grammar.part != b.form.grammar.part ||
            a.route != b.route || a.form.spellingRuleId != b.form.spellingRuleId || a.addonCount != b.addonCount)
          return false;
        for (unsigned i = 0; i < a.addonCount; ++i)
          if (a.addons[i].id != b.addons[i].id || a.addons[i].kind != b.addons[i].kind)
            return false;
        return true;
      };
      if (t.details.size() == formsBefore || !same(t.details.back(), detail)) {
        t.blocks.push_back({t.details.size(), 0});
        ++word.blockCount;
      }
      ++t.blocks.back().count;
      t.details.push_back(detail);
    }
    assert(trace::tLog.readings.size() == t.details.size() - formsBefore);
    t.tags.insert(t.tags.end(), trace::tLog.readings.begin(), trace::tLog.readings.end());
    word.steps = trace::tLog.steps;
    for (const PassRun& run : passesOf(word))
      if (run.ran)
        word.path += (word.path.empty() ? "" : " · ") + std::string{run.pass};
  } else {
    word.path = orEmpty(spokenName("status", byte(word.status)));
  }
  t.words.push_back(std::move(word));
}

void noReadings(const Word& word, std::string_view hint) {
  if (word.status != whitaker::Status::ok)
    html.element("p", "none",
                 "The library returned " + std::string{orEmpty(spokenName("status", byte(word.status)))} + ".");
  else
    html.element("p", "none", "No readings." + std::string{hint});
}

void reading(std::size_t n, const whitaker::Form& form) {
  html.open("div", "reading")
      .element("span", "n", std::to_string(n))
      .element("span", "split", *form.ending ? std::string{form.stemSpelling} + "|" + form.ending : form.stemSpelling)
      .open("div", "body");
}

void readable(const Word& word) {
  Html& h = html;
  if (readingCount(word) == 0 && word.status == whitaker::Status::ok) {
    h.element("p", "none", "Not in whitakerLib's dictionary.");
    return;
  }
  if (readingCount(word) == 0)
    return noReadings(word, "");
  std::size_t n = 0;
  for (const Block& record : recordsOf(word)) {
    const latin::DictionaryWord& e = head(record).dictionaryWord;
    const whitaker::DictionaryWordId id = head(record).dictionaryWordId;
    const std::string route = routeOf(record), addons = addonsLine(record), cls = classInWords(e);
    const std::string meaning{orEmpty(head(record).meaning)};
    h.open("div", "record")
        .element("div", "meaning",
                 id == whitaker::kNoDictionaryWord ? std::to_string(e.numeralValue.value) + " " + meaning : meaning);
    h.element(
        "div", "meta",
        joined({cls.empty() ? orEmpty(head(record).partName) : cls, stemsLine(whitaker::stems(id)),
                route == "direct" ? "" : route, spellingLine(record), addons.empty() ? "" : "add-ons: " + addons}));
    for (std::size_t f = record.first; f < record.first + record.count; ++f) {
      const whitaker::Form& form = loadedText.details[f].form;
      const std::uint8_t stem = loadedText.details[f].stemKey;
      const std::string_view stemWords = stem == 0 ? "" : stemInWords(e.part, stem);
      const std::string grammar = grammarOf(form), words = formInWords(form.grammar);
      const char* age = nameOf<latin::Age>(form.inflectionAge);
      const char* frequency = nameOf<latin::Frequency>(form.inflectionFrequency);
      reading(++n, form);
      h.element("div", "in-words", words.empty() ? grammar : words);
      h.element("div", "meta",
                joined({grammar, stemWords.empty() ? "" : std::string{stemWords} + " (" + std::to_string(stem) + ")",
                        age ? "age " + std::string{age} : "", frequency ? "frequency " + std::string{frequency} : ""}));
      h.close("div").close("div");
    }
    const auto labelled = [](std::string_view label, const char* value) {
      return value ? std::string{label} + " " + value : std::string{};
    };
    h.element("div", "meta entry",
              joined({id == whitaker::kNoDictionaryWord ? "no dictionary word"
                                                        : "dictionary word " + std::to_string(std::to_underlying(id)),
                      orEmpty(kindName(e.part, e.kind.value)), labelled("age", nameOf<latin::Age>(e.age)),
                      labelled("frequency", nameOf<latin::Frequency>(e.frequency)),
                      labelled("area", nameOf<latin::Area>(e.area)),
                      labelled("geography", nameOf<latin::Geography>(e.geography)),
                      labelled("source", nameOf<latin::Source>(e.source)),
                      e.numeralValue.value ? "value " + std::to_string(e.numeralValue.value) : ""}));
    h.close("div");
  }
}

void traced(const Word& word) {
  Html& h = html;
  if (word.status == whitaker::Status::ok) {
    h.element("div", "block-head", "Lookup passes, in order").open("table", "rows");
    for (const PassRun& run : passesOf(word))
      h.open("tr", run.ran ? "" : "dim")
          .element("td", "path", run.pass)
          .element("td", "value", run.ran ? "Ran" : "Not run")
          .element("td", "name", run.ran ? "+" + std::to_string(run.added) + " readings" : "")
          .close("tr");
    h.close("table");
  }
  if (readingCount(word) == 0) {
    noReadings(word, "");
  } else {
    h.element("div", "block-head", "How each reading was found");
    std::size_t n = 0;
    for (const Block& record : recordsOf(word))
      for (std::size_t f = record.first; f < record.first + record.count; ++f) {
        const whitaker::Form& form = loadedText.details[f].form;
        const std::uint8_t traced = trace::stemKey(loadedText.tags[f], form.ending);
        const std::uint8_t stem = traced == trace::kNone ? 0 : traced;
        assert(stem == loadedText.details[f].stemKey);
        const std::string_view stemWords = stem == 0 ? "" : stemInWords(head(record).dictionaryWord.part, stem);
        const std::string addons = addonsLine(record);
        const char* found = foundBy(f);
        reading(++n, form);
        h.element("div", "",
                  joined({"found by " + std::string{found ? found : "—"}, "route " + routeOf(record),
                          stem == 0 ? ""
                                    : "stem key " + std::to_string(stem) +
                                          (stemWords.empty() ? "" : " (" + std::string{stemWords} + ")")}));
        h.element("div", "meta",
                  joined({spellingLine(record), addons.empty() ? "" : "add-ons, outermost first: " + addons}));
        h.close("div").close("div");
      }
  }
  h.element("p", "meta",
            joined({"status " + std::string{orEmpty(spokenName("status", byte(word.status)))},
                    trimmed(word) ? "trimmed yes" : "trimmed no"}));
}

struct Measure {
  std::string name;
  std::string path;
  std::vector<const char*> words;
  std::size_t occurrences = 0;
  std::vector<double> samples;
};

struct Timings {
  std::vector<Measure> measures;
  std::size_t from = 0, to = 0;
  bool valid = false;
};

Timings onceTimings;

[[nodiscard]] Timings measuresOf(std::size_t from, std::size_t to) {
  Timings t;
  t.from = from;
  t.to = to;
  t.valid = true;
  const std::vector<Group> groups = groupsOf(from, to);
  Measure every;
  every.name = "Every occurrence";
  for (std::size_t i = from; i <= to && i < loadedText.occurrences.size(); ++i)
    every.words.push_back(loadedText.words[loadedText.occurrences[i]].spelling.c_str());
  const std::size_t occurrences = every.words.size();
  t.measures.push_back(std::move(every));
  if (groups.size() != occurrences) {
    Measure distinct;
    distinct.name = "Distinct as written";
    for (const Group& g : groups)
      distinct.words.push_back(loadedText.words[g.word].spelling.c_str());
    t.measures.push_back(std::move(distinct));
  }
  const std::size_t paths = t.measures.size();
  for (const Group& g : groups) {
    const Word& word = loadedText.words[g.word];
    if (word.status != whitaker::Status::ok)
      continue;
    auto at = std::ranges::find(t.measures.begin() + static_cast<std::ptrdiff_t>(paths), t.measures.end(), word.path,
                                &Measure::path);
    if (at == t.measures.end()) {
      t.measures.emplace_back();
      t.measures.back().path = word.path;
      at = t.measures.end() - 1;
    }
    at->words.push_back(word.spelling.c_str());
    at->occurrences += g.count;
  }
  return t;
}

[[nodiscard]] double clockStep() {
  static const double step = [] {
    double best = 1e9;
    for (int n = 0; n < 5; ++n) {
      const double from = emscripten_get_now();
      double to = from;
      while (!(to > from))
        to = emscripten_get_now();
      best = std::min(best, to - from);
    }
    return best;
  }();
  return step;
}

[[nodiscard]] double timed(std::span<const char* const> words, double minimumMs) {
  static whitaker::Result timing;
  if (words.empty())
    return 0;
  const double start = emscripten_get_now();
  double elapsed = 0;
  unsigned runs = 0;
  do {
    for (std::size_t i = 0; i < words.size(); ++i)
      static_cast<void>(i == 0 ? whitaker::analyze(timing, words[i]) : whitaker::append(timing, words[i]));
    ++runs;
    elapsed = emscripten_get_now() - start;
  } while (elapsed < minimumMs);
  return elapsed * 1e3 / runs / static_cast<double>(words.size());
}

[[nodiscard]] double median(std::vector<double> list) {
  std::ranges::sort(list);
  return list[list.size() / 2];
}

void headRow(std::initializer_list<std::string_view> names) {
  html.open("tr", "dim");
  for (const std::string_view name : names)
    html.element("td", "", name);
  html.close("tr");
}

void speeds(const Timings& t) {
  Html& h = html;
  h.open("table", "rows");
  headRow({"", "Calls", "Per word", "Total"});
  for (const Measure& m : t.measures) {
    if (!m.path.empty())
      continue;
    const bool timedYet = !m.samples.empty();
    const double per = timedYet ? median(m.samples) : 0;
    h.open("tr").element("td", "", m.name).element("td", "", count(m.words.size()));
    h.element("td", "", timedYet ? duration(per) : "…");
    h.element("td", "", timedYet ? duration(per * static_cast<double>(m.words.size())) : "").close("tr");
  }
  h.close("table");
}

void costs(const Timings& t) {
  Html& h = html;
  struct Row {
    const Measure* m;
    double per, time;
  };
  std::vector<Row> rows;
  double total = 0;
  for (const Measure& m : t.measures)
    if (!m.path.empty()) {
      const double per = m.samples.empty() ? 0 : median(m.samples);
      rows.push_back({&m, per, per * static_cast<double>(m.occurrences)});
      total += rows.back().time;
    }
  if (!(total > 0))
    total = 1;
  std::ranges::stable_sort(rows, std::ranges::greater{}, &Row::time);
  h.open("table", "rows");
  headRow({"Share", "", "Per word", "Words", "Distinct"});
  for (const Row& r : rows) {
    const bool timedYet = !r.m->samples.empty();
    std::string bar;
    for (long i = std::max(1L, std::lround(12 * r.time / total)); i > 0; --i)
      bar += "▇";
    h.open("tr").element("td", "", timedYet ? fixed(100 * r.time / total, 1) + "%" : "…");
    h.element("td", "bar", bar).element("td", "", timedYet ? duration(r.per) : "");
    h.element("td", "", count(r.m->occurrences));
    h.element("td", "", count(r.m->words.size())).close("tr");
    h.open("tr").element("td", "", "").open("td", "value path-line", "colspan=\"4\"").add(r.m->path);
    h.close("td").close("tr");
  }
  h.close("table");
}

void counts() {
  Html& h = html;
  struct Tally {
    std::string value;
    std::size_t occurrences = 0, distinct = 0;
  };
  constexpr std::string_view kFields[] = {"part", "case",  "number", "gender",   "tense",   "voice",
                                          "mood", "route", "add-on", "spelling", "readings"};
  std::vector<std::vector<Tally>> tallies(std::size(kFields));
  // NOTE: Each value's place in its list, found by hash: a book's spellings, looked for one by one in the list itself,
  //  took 4.7 s for Livy.
  std::vector<std::unordered_map<std::string, std::size_t>> places(std::size(kFields));
  std::size_t occurrences = 0;
  for (const Group& g : selectedGroups) {
    occurrences += g.count;
    const Word& word = loadedText.words[g.word];
    std::vector<Value> values;
    for (const Block& record : recordsOf(word))
      for (std::size_t f = record.first; f < record.first + record.count; ++f)
        readingValues(record, f, values);
    values.push_back({"spelling", lower(word.spelling)});
    values.push_back({"readings", std::to_string(readingCount(word))});
    std::vector<std::pair<std::size_t, std::string>> seen;
    for (Value& v : values) {
      const auto field = static_cast<std::size_t>(std::ranges::find(kFields, v.field) - std::begin(kFields));
      if (std::ranges::find(seen, std::pair{field, v.value}) != seen.end())
        continue;
      seen.emplace_back(field, v.value);
      std::vector<Tally>& list = tallies[field];
      const auto [place, added] = places[field].try_emplace(v.value, list.size());
      if (added)
        list.push_back(Tally{std::move(v.value), 0, 0});
      Tally& tally = list[place->second];
      tally.occurrences += g.count;
      ++tally.distinct;
    }
  }
  // NOTE: Spellings as the text's numbers count them (page.js, case folded), and as written, as each is looked up.
  std::unordered_set<std::string> folded;
  for (const Group& g : selectedGroups)
    folded.insert(lower(loadedText.words[g.word].spelling));
  h.element("div", "block-head",
            "Counts: " + count(occurrences) + " words, " + count(folded.size()) + " spellings (" +
                count(selectedGroups.size()) + " as written)");
  h.open("table", "rows counts");
  for (std::size_t field = 0; field < std::size(kFields); ++field) {
    std::vector<Tally>& list = tallies[field];
    if (list.empty())
      continue;
    if (kFields[field] == "readings")
      std::ranges::stable_sort(list, {}, [](const Tally& t) { return std::stoul(t.value); });
    else
      std::ranges::stable_sort(list, std::ranges::greater{}, &Tally::occurrences);
    const std::size_t shown = kFields[field] == "spelling" ? std::min<std::size_t>(40, list.size()) : list.size();
    h.open("tr").element("td", "path", kFields[field]).open("td", "value");
    for (const Tally& t : std::span{list}.first(shown)) {
      h.open("span", "value").add(t.value + " " + count(t.occurrences));
      if (t.distinct != t.occurrences)
        h.element("span", "meta", " (" + count(t.distinct) + ")");
      h.close("span");
    }
    if (list.size() > shown)
      h.add(" and " + count(list.size() - shown) + " more");
    h.close("td").close("tr");
  }
  h.close("table");
}

void traceCosts() {
  const bool once = onceTimings.valid && onceTimings.from == selectedFrom && onceTimings.to == selectedTo;
  const Timings fresh = once ? Timings{} : measuresOf(selectedFrom, selectedTo);
  html.element("div", "block-head", "Where the time goes");
  costs(once ? onceTimings : fresh);
}

void statistics() {
  Html& h = html;
  const bool once = onceTimings.valid && onceTimings.from == selectedFrom && onceTimings.to == selectedTo;
  const Timings fresh = once ? Timings{} : measuresOf(selectedFrom, selectedTo);
  const Timings& t = once ? onceTimings : fresh;
  h.element("div", "block-head", "Measuring the lookup");
  speeds(t);
  h.element("p", "meta",
            "The measurement starts with analyze() for the first word and uses append() for each subsequent word. "
            "The test is repeated until enough time has passed to measure it reliably, since browsers limit the "
            "precision of their clocks.");
  h.element("p", "meta",
            "Tracing is enabled during these measurements. The time spent fetching the text and drawing "
            "the page is not included.");
  counts();
}

} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE const char* inspector_load(const char* words) {
  Text& t = loadedText;
  whitaker::clear(t.result);
  t.details.clear();
  t.blocks.clear();
  t.words.clear();
  t.tags.clear();
  t.occurrences.clear();
  onceTimings = {};
  std::unordered_map<std::string, std::uint32_t> ids;
  for (const auto part : std::views::split(std::string_view{words}, '\n')) {
    std::string spelling{part.begin(), part.end()};
    if (spelling.empty())
      continue;
    const auto [at, added] = ids.try_emplace(spelling, static_cast<std::uint32_t>(t.words.size()));
    if (added)
      lookUp(spelling);
    t.occurrences.push_back(at->second);
  }
  reply.clear();
  for (const std::uint32_t w : t.occurrences)
    reply += readingCount(t.words[w]) ? '1' : '0';
  selectedFrom = selectedTo = 0;
  selectedGroups = groupsOf(0, 0);
  return reply.c_str();
}

extern "C" EMSCRIPTEN_KEEPALIVE unsigned inspector_select(unsigned from, unsigned to) {
  selectedFrom = std::min(from, to);
  selectedTo = std::max(from, to);
  selectedGroups = groupsOf(selectedFrom, selectedTo);
  return static_cast<unsigned>(selectedGroups.size());
}

extern "C" EMSCRIPTEN_KEEPALIVE const char* inspector_view(unsigned view, unsigned first, unsigned count) {
  Html& h = html;
  h.clear();
  if (loadedText.words.empty())
    return h.text();
  if (view == 3) {
    statistics();
    return h.text();
  }
  if (view == 2 && first == 0)
    traceCosts();
  const std::size_t end = std::min<std::size_t>(selectedGroups.size(), std::size_t{first} + count);
  for (std::size_t i = first; i < end; ++i) {
    const Group& g = selectedGroups[i];
    const Word& word = loadedText.words[g.word];
    if (selectedFrom != selectedTo)
      h.open("div", "group-head")
          .add(word.spelling)
          .element("span", "count", " ×" + std::to_string(g.count))
          .close("div");
    if (view == 0)
      readable(word);
    else
      traced(word);
  }
  return h.text();
}

// INFO: For the sentence window's guesses (site/sentence/highlights.js, sentence.js): four hex digits a word in
//  the text, its flags, from its readings. Which reading is right in its sentence is the page's guess, not this.
constexpr unsigned kFinite = 1; // some reading is a verb with a person (indicative, subjunctive, imperative)
constexpr unsigned kInfinitive = 4;
constexpr unsigned kUnknown = 8;
constexpr unsigned kParticiple = 16;
constexpr unsigned kQue = 32; // some reading carries the tackon -que
constexpr unsigned kAblative = 64;
constexpr unsigned kNotVerb = 128;        // some reading is another part of speech (a participle counts as one)
constexpr unsigned kOnlyNominative = 256; // every reading is a nominative: a likely subject
constexpr unsigned kNominative = 512;     // some reading is

extern "C" EMSCRIPTEN_KEEPALIVE const char* inspector_marks() {
  std::vector<unsigned> marks(loadedText.words.size(), 0);
  for (std::size_t w = 0; w < loadedText.words.size(); ++w) {
    unsigned m = 0;
    std::size_t nominative = 0;
    std::size_t all = 0;
    for (const Block& record : recordsOf(loadedText.words[w]))
      for (std::size_t f = record.first; f < record.first + record.count; ++f) {
        const whitaker::ReadingDetail& d = loadedText.details[f];
        const latin::Mood mood = d.form.grammar.mood;
        const bool isFinite =
            d.form.grammar.part == latin::Part::verb &&
            (mood == latin::Mood::indicative || mood == latin::Mood::subjunctive || mood == latin::Mood::imperative);
        nominative += d.form.grammar.caseOf == latin::Case::nominative;
        m |= d.form.grammar.caseOf == latin::Case::nominative ? kNominative : 0;
        ++all;
        m |= isFinite ? kFinite : 0;
        m |= mood == latin::Mood::infinitive ? kInfinitive : 0;
        m |= d.form.grammar.part == latin::Part::participle ? kParticiple : 0;
        m |= d.form.grammar.caseOf == latin::Case::ablative ? kAblative : 0;
        m |= d.form.grammar.part != latin::Part::verb ? kNotVerb : 0;
        for (const whitaker::AddonStep& step : std::span{d.addons, d.addonCount})
          m |= orEmpty(whitaker::addonSpelling(step.id)) == "que" ? kQue : 0;
      }
    m |= all == 0 ? kUnknown : 0;
    m |= all && nominative == all ? kOnlyNominative : 0;
    marks[w] = m;
  }
  reply.clear();
  constexpr std::string_view kHex = "0123456789abcdef";
  for (const std::uint32_t w : loadedText.occurrences)
    for (int shift = 12; shift >= 0; shift -= 4)
      reply += kHex[(marks[w] >> shift) & 15];
  return reply.c_str();
}

extern "C" EMSCRIPTEN_KEEPALIVE unsigned inspector_time() {
  if (loadedText.occurrences.empty() ||
      (onceTimings.valid && onceTimings.from == selectedFrom && onceTimings.to == selectedTo))
    return 0;
  onceTimings = measuresOf(selectedFrom, selectedTo);
  for (Measure& m : onceTimings.measures)
    m.samples.push_back(timed(m.words, std::max(m.path.empty() ? 30.0 : 10.0, 20 * clockStep())));
  return 1;
}
