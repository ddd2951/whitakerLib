// INFO: The page's library, as JSON. read_text(words), one word a line, reads a text once into one Result:
//   {"known": "0110…", "readings": [[readings, words], …], "parts": [[part, words], …]}
//  word(i) gives word i's readings in the library's order, with the fields popup.js reads, or {"error"}.
//  api(word, binding) gives the api window's calls in C++, C or Python (binding 0, 1, 2; api_view.inc):
//   {"status", "calls": [{"function", "call", "note", "rows": [[field, value], …]}, …]}
//  A returned string stays valid until the next call.
// NOTE: Every name a reader sees comes from names.hpp, as in the Firefox extension.
#include "names.hpp"

#include <whitaker.hpp>

#include <emscripten/emscripten.h>

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstdio>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kNotRead = ~0u;

whitaker::Result result;
std::vector<std::uint32_t> wordIds; // a word of the text, its spelling's WordId in result (kNotRead if not taken)
std::string reply;

// NOTE: The JSON is written in its fixed shape: each value followed by a comma, which close() takes back.
void text(std::string_view value) {
  reply += '"';
  for (const char c : value) {
    if (c == '"' || c == '\\')
      reply += '\\';
    if (static_cast<unsigned char>(c) < 0x20) {
      char escaped[8]{};
      std::snprintf(escaped, sizeof escaped, "\\u%04x", c);
      reply += escaped;
    } else {
      reply += c;
    }
  }
  reply += "\",";
}
void key(std::string_view name) {
  reply += '"';
  reply += name;
  reply += "\":";
}
void text(std::string_view name, std::string_view value) {
  key(name);
  text(value);
}
void text(std::string_view name, const char* value) { text(name, std::string_view{value ? value : ""}); }
void number(std::string_view name, unsigned value) {
  key(name);
  reply += std::to_string(value) + ',';
}
void flag(std::string_view name, bool value) {
  key(name);
  reply += value ? "true," : "false,";
}
void close(char bracket) {
  if (reply.back() == ',')
    reply.pop_back();
  reply += bracket;
  reply += ',';
}

// INFO: A value as the binding writes it, for api_view.inc's detailRows and the api window's own rows.
// NOTE: Rows is calls, not data: api_view.inc (codegen) calls rows.text(), rows.number() and the rest on it.
enum class Binding : unsigned char {
  cpp,
  c,
  python,
};
Binding binding = Binding::cpp;

struct Rows {
  void row(std::string_view field, std::string_view value) {
    reply += '[';
    ::text(field);
    ::text(value);
    close(']');
  }
  void number(std::string_view field, unsigned long long value) { row(field, std::to_string(value)); }
  void flag(std::string_view field, bool value) {
    row(field, binding == Binding::python ? (value ? "True" : "False") : (value ? "true" : "false"));
  }
  void text(std::string_view field, const char* value) {
    if (value == nullptr)
      return row(field, binding == Binding::python ? "None" : binding == Binding::c ? "NULL" : "nullptr");
    const char quote = binding == Binding::python ? '\'' : '"';
    row(field, quote + std::string{value} + quote);
  }
  void enumerated(std::string_view field, std::string_view type, const char* cName, const char* name,
                  const char* label = nullptr) {
    const std::string named = binding == Binding::c        ? std::string{cName}
                              : binding == Binding::python ? std::string{type} + '.' + name
                                                           : "latin::" + std::string{type} + "::" + name;
    row(field, label == nullptr ? named : named + (binding == Binding::python ? "  # " : "  // ") + label);
  }
};

#include "api_view.inc"

void reading(const whitaker::ReadingDetail& d, const std::string_view dictionaryForm) {
  reply += '{';
  text("part", names::part(d.form.grammar.part));
  text("dictionaryPart", names::part(d.dictionaryWord.part));
  text("class", names::wordClass(d));
  text("grammar", names::grammar(d.form.grammar));
  // NOTE: The grammar again in three parts, so the popup can list a form's cases under one line: the case, the gender
  //  and number it agrees in, and the rest. Each worded by names.hpp, as the whole is.
  latin::Analysis agreement{};
  agreement.gender = d.form.grammar.gender;
  agreement.number = d.form.grammar.number;
  latin::Analysis rest = d.form.grammar;
  rest.caseOf = {};
  rest.gender = {};
  rest.number = {};
  text("case", names::named(d.form.grammar.caseOf));
  text("agreement", names::grammar(agreement));
  text("rest", names::grammar(rest));
  text("stem", d.form.stemSpelling);
  text("ending", d.form.ending);
  text("meaning", d.meaning);
  number("stemKey", d.stemKey);
  // NOTE: A dictionary word's stems by key, as DICTLINE spells them; none for a Roman numeral.
  key("stems");
  reply += '[';
  const auto stems = whitaker::stems(d.dictionaryWordId);
  for (const whitaker::Stem& stem : std::span{stems.stems, stems.count}) {
    reply += '{';
    number("key", stem.key);
    text("spelling", stem.spelling);
    close('}');
  }
  close(']');
  text("dictionaryForm", dictionaryForm);
  number("numeral", d.dictionaryWordId == whitaker::kNoDictionaryWord ? d.dictionaryWord.numeralValue.value : 0);
  // NOTE: A dictionary numeral's value and the form's sort (tertiam: 3, ordinal), so the popup can say "3rd": the
  //  meaning WORDS gives is the cardinal's ("three;").
  number("number", d.dictionaryWordId == whitaker::kNoDictionaryWord ? 0 : d.dictionaryWord.numeralValue.value);
  text("sort", cppName(d.form.grammar.numeralSort));
  key("addons");
  reply += '[';
  for (const whitaker::AddonStep& step : std::span{d.addons, d.addonCount}) {
    reply += '{';
    text("spelling", whitaker::addonSpelling(step.id));
    text("meaning", whitaker::addonMeaning(step.id));
    text("kind", cppName(step.kind));
    close('}');
  }
  close(']');
  // NOTE: How the reading was found (the popup groups by it): the route, the contraction restored (empty for none),
  //  and the respelling used, if any, as the rule's two spellings.
  text("route", cppName(d.route));
  text("syncope", d.form.derivation.syncope == latin::Syncope::NONE ? "" : cppName(d.form.derivation.syncope));
  flag("adverb", d.form.derivation.adverb);
  key("respelling");
  if (d.form.spellingRuleId == 0) {
    reply += "null,";
  } else {
    const whitaker::SpellingRule rule = whitaker::spellingRule(d.form.spellingRuleId);
    reply += '{';
    text("first", rule.first);
    text("second", rule.second);
    flag("reversed", rule.reversed);
    close('}');
  }
  close('}');
}

} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE const char* read_text(const char* words) {
  whitaker::clear(result);
  wordIds.clear();
  std::unordered_map<std::string_view, std::uint32_t> ids;
  for (const auto part : std::views::split(std::string_view{words}, '\n')) {
    const std::string_view spelling{part.begin(), part.end()};
    if (spelling.empty())
      continue;
    const auto [at, added] = ids.try_emplace(spelling, static_cast<std::uint32_t>(whitaker::wordCount(result)));
    if (added && whitaker::append(result, spelling) != whitaker::Status::ok)
      at->second = kNotRead;
    wordIds.push_back(at->second);
  }

  std::map<std::uint32_t, std::uint32_t> byReadings;
  std::map<std::string_view, std::uint32_t> byPart;
  reply = R"({"known":")";
  for (const std::uint32_t id : wordIds) {
    const auto readings =
        id == kNotRead ? std::span<const whitaker::Reading>{} : whitaker::readings(result, whitaker::WordId{id});
    reply += readings.empty() ? '0' : '1';
    ++byReadings[static_cast<std::uint32_t>(readings.size())];
    // NOTE: A part from the packed reading, as detail() gives it; a packon counts as the pronoun it reads as.
    std::uint32_t parts = 0;
    for (const whitaker::Reading& r : readings)
      parts |= 1u << std::to_underlying(r.part == latin::Part::packon ? latin::Part::pronoun : r.part);
    for (; parts; parts &= parts - 1)
      if (const auto name = names::part(static_cast<latin::Part>(std::countr_zero(parts))); !name.empty())
        ++byPart[name];
  }
  reply += R"(","readings":[)";
  for (const auto [n, count] : byReadings)
    reply += '[' + std::to_string(n) + ',' + std::to_string(count) + "],";
  close(']');
  key("parts");
  reply += '[';
  std::vector<std::pair<std::string_view, std::uint32_t>> parts{byPart.begin(), byPart.end()};
  std::ranges::stable_sort(parts, std::ranges::greater{}, &std::pair<std::string_view, std::uint32_t>::second);
  for (const auto& [name, count] : parts) {
    reply += '[';
    text(name);
    reply += std::to_string(count) + "],";
  }
  close(']');
  close('}');
  reply.pop_back();
  return reply.c_str();
}

namespace {

// INFO: A word's answer, as word(i) and lookup(spelling) give it.
const char* answer(const whitaker::Result& from, const whitaker::WordId w) {
  reply = "{";
  text("word", whitaker::spelling(from, w));
  flag("trimmed", whitaker::trimmed(from, w));
  key("readings");
  reply += '[';
  for (std::size_t r = 0; r < whitaker::readingCount(from, w); ++r) {
    const auto dictionaryForm = whitaker::dictionaryForm(from, w, r);
    reading(whitaker::detail(from, w, r), dictionaryForm.text);
  }
  close(']');
  close('}');
  reply.pop_back();
  return reply.c_str();
}

const char* notTaken() {
  reply = R"({"error":"not a word the library takes"})";
  return reply.c_str();
}

// NOTE: A Result of its own for one word (lookup, api), so the text's stays as it is.
whitaker::Result one;

} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE const char* word(unsigned i) {
  return i >= wordIds.size() || wordIds[i] == kNotRead ? notTaken() : answer(result, whitaker::WordId{wordIds[i]});
}

// INFO: One word's answer, outside the text: the popup in Explain this answer.
extern "C" EMSCRIPTEN_KEEPALIVE const char* lookup(const char* spelling) {
  return whitaker::analyze(one, spelling) == whitaker::Status::ok ? answer(one, whitaker::WordId{0}) : notTaken();
}

extern "C" EMSCRIPTEN_KEEPALIVE const char* api(const char* spelling, unsigned bindingIndex) {
  binding = static_cast<Binding>(std::min(bindingIndex, 2u));
  const auto call = [](std::string_view function, std::string_view note) {
    reply += '{';
    text("function", function);
    for (const auto& calls : kCalls)
      if (std::string_view{calls[0]}.starts_with("whitaker::" + std::string{function} + "("))
        text("call", calls[std::to_underlying(binding)]);
    text("note", note);
    key("rows");
    reply += '[';
  };
  const auto end = [] {
    close(']');
    close('}');
  };
  Rows rows;
  reply = "{";
  const whitaker::Status status = whitaker::analyze(one, spelling);
  text("status", cppName(status));
  key("calls");
  reply += '[';
  if (status == whitaker::Status::ok) {
    const whitaker::WordId w{0};
    call("spelling", "");
    rows.text("", std::string{whitaker::spelling(one, w)}.c_str());
    end();
    call("trimmed", "");
    rows.flag("", whitaker::trimmed(one, w));
    end();
    call("readingCount", "");
    rows.number("", whitaker::readingCount(one, w));
    end();
    std::vector<whitaker::DictionaryWordId> words;
    std::vector<std::uint16_t> addons;
    for (std::size_t r = 0; r < whitaker::readingCount(one, w); ++r) {
      const whitaker::ReadingDetail d = whitaker::detail(one, w, r);
      call("detail", "readingIndex " + std::to_string(r));
      detailRows(rows, d);
      end();
      if (d.dictionaryWordId != whitaker::kNoDictionaryWord && !std::ranges::contains(words, d.dictionaryWordId))
        words.push_back(d.dictionaryWordId);
      for (const whitaker::AddonStep& step : std::span{d.addons, d.addonCount})
        if (!std::ranges::contains(addons, step.id))
          addons.push_back(step.id);
    }
    for (const whitaker::DictionaryWordId id : words) {
      call("stems", "id " + std::to_string(std::to_underlying(id)));
      const auto stems = whitaker::stems(id);
      for (std::size_t s = 0; s < stems.count; ++s) {
        const std::string at = "stems.stems[" + std::to_string(s) + "]";
        rows.number(at + ".key", stems.stems[s].key);
        rows.text(at + ".spelling", stems.stems[s].spelling);
      }
      rows.number("stems.count", stems.count);
      end();
    }
    for (const std::uint16_t id : addons) {
      call("addonSpelling", "id " + std::to_string(id));
      rows.text("", whitaker::addonSpelling(id));
      end();
      call("addonMeaning", "id " + std::to_string(id));
      rows.text("", whitaker::addonMeaning(id));
      end();
    }
  }
  close(']');
  close('}');
  reply.pop_back();
  return reply.c_str();
}
