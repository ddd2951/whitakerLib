#pragma once

// INFO: Every name the extension shows a reader: the library's labels (latin::label), and the few places where the
//  popup says more than a label does ("common gender", "relative pronoun").

#include <latin_generated.hpp>
#include <whitaker.hpp>

#include <cstddef>
#include <string>
#include <string_view>

namespace names {

constexpr std::string_view kNth[] = {"", "1st", "2nd", "3rd", "4th", "5th"};

// INFO: The reader's words for a value; empty for the library's "any" (value 0).
template <typename E> [[nodiscard]] std::string_view named(E value) {
  const char* words = latin::label(value);
  return static_cast<unsigned char>(value) == 0 || words == nullptr ? std::string_view{} : words;
}

// NOTE: A packon (quidam's qui-) reads as the pronoun it is.
[[nodiscard]] inline std::string_view part(latin::Part value) {
  return value == latin::Part::packon ? "pronoun" : named(value);
}

// INFO: The inflection as a reader says it: "perfect, active, indicative, 3rd person, singular".
[[nodiscard]] inline std::string grammar(const latin::Analysis& a) {
  std::string out;
  const auto put = [&out](std::string_view words) {
    if (words.empty())
      return;
    if (!out.empty())
      out += ", ";
    out += words;
  };
  put(named(a.comparison));
  put(named(a.tense));
  put(named(a.voice));
  // NOTE: A participle's mood is the part already shown.
  put(a.part == latin::Part::participle ? "" : named(a.mood));
  if (a.person.value != 0 && a.person.value < std::size(kNth))
    put(std::string(kNth[a.person.value]) + " person");
  put(a.gender == latin::Gender::common ? "common gender" : named(a.gender));
  put(named(a.number));
  put(named(a.caseOf));
  put(a.numeralSort == latin::NumeralSort::adverbial ? "adverbial numeral" : named(a.numeralSort));
  return out;
}

// INFO: The class as a reader names it, as the Kindle plugin does: a pronoun's kind (a compound one's too, as quidam),
//  a verb's conjugation and whether it is deponent, a noun's or an adjective's declension (a comparative's is the
//  3rd and a superlative's the 1st and 2nd, whatever its positive's). WORDS' other classes (Greek adjectives, verbs
//  past 4) say nothing to a reader.
// NOTE: A pronoun's kind and a compound pronoun's (packon) are numbered differently: each is read as its own type.
[[nodiscard]] inline std::string wordClass(const whitaker::ReadingDetail& d) {
  using latin::Part;
  const Part form = d.form.grammar.part;
  const unsigned which = d.form.grammar.which.value;
  const auto pronoun = [](std::string_view kind, bool adjectival) {
    return adjectival ? std::string{"pronominal adjective"} : kind.empty() ? "" : std::string(kind) + " pronoun";
  };
  if (d.dictionaryWord.part == Part::pronoun) {
    const latin::PronounKind kind{d.dictionaryWord.kind.value};
    return pronoun(named(kind), kind == latin::PronounKind::adjectival);
  }
  if (d.dictionaryWord.part == Part::packon) {
    const latin::PackonKind kind{d.dictionaryWord.kind.value};
    return pronoun(named(kind), kind == latin::PackonKind::adjectival);
  }
  if (which == 0 || form == Part::numeral)
    return {};
  if (form == Part::verb || form == Part::supine || form == Part::participle) {
    std::string out = which <= 4 ? std::string(kNth[which]) + " conjugation" : which < 9 ? "irregular verb" : "";
    const latin::VerbKind kind{d.dictionaryWord.kind.value};
    if (d.dictionaryWord.part == Part::verb && kind == latin::VerbKind::deponent)
      out += out.empty() ? "deponent" : ", deponent";
    if (d.dictionaryWord.part == Part::verb && kind == latin::VerbKind::semideponent)
      out += out.empty() ? "semideponent" : ", semideponent";
    return out;
  }
  if (form == Part::adjective && d.form.grammar.comparison == latin::Comparison::comparative)
    return "3rd declension";
  if (form == Part::adjective && d.form.grammar.comparison == latin::Comparison::superlative)
    return "1st and 2nd declension";
  if (form == Part::adjective)
    return which == 1 ? "1st and 2nd declension" : which == 3 ? "3rd declension" : "";
  return which < std::size(kNth) ? std::string(kNth[which]) + " declension" : "";
}

} // namespace names
