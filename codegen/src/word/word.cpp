#include "word.hpp"

#include <main_config.hpp>

#include "error/error.hpp"
#include "source/schemes.hpp"
#include "util/reflect_util.hpp"
#include "word/field.hpp"
#include "word/report.hpp"
#include "word/split.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <map>
#include <print>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

namespace {

namespace dictlineScheme = source::scheme::dictline;
namespace inflectsScheme = source::scheme::inflects;
namespace uniquesScheme = source::scheme::uniques;
namespace addonsScheme = source::scheme::addons;
using word::internal::counts;
using word::internal::Fields;
using word::internal::name;
using word::internal::number;
using word::internal::split;

struct Labels {
  latin::Age age;
  latin::Area area;
  latin::Geography geography;
  latin::Frequency frequency;
  latin::Source source;
};

struct DictlineEntry {
  word::Stem stem1;
  word::Stem stem2;
  word::Stem stem3;
  word::Stem stem4;
  latin::Entry grammar;
  Labels labels;
  std::string_view senses;
  std::string_view packon;
};

struct InflectsEntry {
  latin::Inflection grammar;
  latin::StemKey stemKey;
  latin::CharacterCount characterCount;
  std::string_view ending;
  latin::Age age;
  latin::Frequency frequency;
};

struct UniquesEntry {
  std::string_view form;
  latin::Entry grammar;
  latin::Inflection inflection;
  Labels labels;
  std::string_view senses;
};

struct AddonsEntry {
  latin::Addon addon;
  std::string_view meaning;
  latin::AddonKind kind;
};

std::array<DictlineEntry, word::kDictlineWords> dictline;
std::array<InflectsEntry, word::kInflectsWords> inflects;
std::array<UniquesEntry, uniquesScheme::kEntriesPerFile> uniques;
std::array<AddonsEntry, addonsScheme::kEntriesPerFile> addons;

template <typename T> struct Parsed {
  T value;
  std::size_t fields;
};

template <typename Index>
std::string where(std::string_view path, Index entry) {
  return std::string{path} + ":" + std::to_string(source::lineNumber(entry));
}

template <typename E>
E letter(char c, std::string_view name, std::string_view where) {
  if (const auto value = util::trySvToEnum<E>(std::string_view{&c, 1}))
    return *value;
  error::fatal(std::string{where} + ": '" + c + "' is not a " +
               std::string{name});
}

template <typename E>
E letter(std::string_view field, std::string_view name,
         std::string_view where) {
  if (field.size() != 1)
    error::fatal(std::string{where} + ": '" + std::string{field} +
                 "' is not a " + std::string{name});
  return letter<E>(field.front(), name, where);
}

consteval bool partsAreNamed(std::span<const std::string_view> parts) {
  for (const std::string_view part : parts)
    if (!util::trySvToEnum<latin::Part>(part))
      return false;
  return true;
}
static_assert(partsAreNamed(dictlineScheme::kParts),
              "dictline scheme: kParts holds a token that is not a part");
static_assert(partsAreNamed(inflectsScheme::kParts),
              "inflects scheme: kParts holds a token that is not a part");
static_assert(partsAreNamed(uniquesScheme::kParts),
              "uniques scheme: kParts holds a token that is not a part");
static_assert(partsAreNamed(addonsScheme::kParts),
              "addons scheme: kParts holds a token that is not a part");

latin::Part part(std::span<const std::string_view> parts, std::string_view file,
                 std::string_view token, std::string_view where) {
  for (const std::string_view part : parts)
    if (part == token)
      return *util::trySvToEnum<latin::Part>(token);
  error::fatal(std::string{where} + ": not " + std::string{file} + " part: '" +
               std::string{token} + "'");
}

template <std::size_t max>
void expect(const Fields<max>& f, std::size_t at, std::size_t arity,
            latin::Part part, std::string_view where) {
  if (f.count < at + arity)
    error::fatal(std::string{where} + ": " + std::string{util::enumToSv(part)} +
                 " states " + std::to_string(arity) +
                 " grammar fields, record has " + std::to_string(f.count - at));
}

template <std::size_t max>
void done(const Fields<max>& f, std::size_t at, std::string_view where) {
  if (f.count != at)
    error::fatal(std::string{where} + ": " + std::to_string(f.count - at) +
                 " fields left after the grammar");
}

template <std::size_t max>
void labelsFollow(const Fields<max>& f, std::size_t at, std::size_t labels,
                  std::string_view after, std::string_view where) {
  if (f.count != at + labels)
    error::fatal(std::string{where} + ": " + std::to_string(f.count - at) +
                 " fields after the " + std::string{after} +
                 ", the format states " + std::to_string(labels));
}

std::string_view trimmed(std::string_view text, char padding) {
  const std::size_t end = text.find_last_not_of(padding);
  return end == std::string_view::npos ? std::string_view{}
                                       : text.substr(0, end + 1);
}

void letters(std::string_view text, std::string_view what,
             std::string_view where) {
  for (const char c : text)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
      error::fatal(std::string{where} + ": " + std::string{what} + " '" +
                   std::string{text} + "' is not letters");
}

std::string_view printable(std::string_view text, std::size_t minLength,
                           char lowest, char highest, std::string_view what,
                           std::string_view where) {
  if (text.size() < minLength)
    error::fatal(std::string{where} + ": entry has no " + std::string{what});
  const auto low = std::ranges::find_if(text, [lowest, highest](char c) {
    const auto byte = static_cast<unsigned char>(c);
    return byte < static_cast<unsigned char>(lowest) ||
           byte > static_cast<unsigned char>(highest);
  });
  if (low != text.end())
    error::fatal(std::string{where} + ": " + std::string{what} +
                 " holds a byte outside printable ASCII at offset " +
                 std::to_string(low - text.begin()));
  return text;
}

namespace lexeme {

namespace scheme = dictlineScheme;
using namespace latin;

template <std::size_t max>
Noun noun(const Fields<max>& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kNounFields, latin::Part::N, where);
  return {number<latin::Declension>(f, at, where),
          number<latin::Variant>(f, at + 1, where),
          name<latin::Gender>(f, at + 2, where),
          name<latin::NounKind>(f, at + 3, where)};
}

template <std::size_t max>
Pronoun pronoun(const Fields<max>& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kPronounFields, latin::Part::PRON, where);
  return {number<latin::Declension>(f, at, where),
          number<latin::Variant>(f, at + 1, where),
          name<latin::PronounKind>(f, at + 2, where)};
}

template <std::size_t max>
Verb verb(const Fields<max>& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kVerbFields, latin::Part::V, where);
  return {number<latin::Conjugation>(f, at, where),
          number<latin::Variant>(f, at + 1, where),
          name<latin::VerbKind>(f, at + 2, where)};
}

template <std::size_t max>
Adjective adjective(const Fields<max>& f, std::size_t at,
                    std::string_view where) {
  expect(f, at, scheme::kAdjectiveFields, latin::Part::ADJ, where);
  return {number<latin::Declension>(f, at, where),
          number<latin::Variant>(f, at + 1, where),
          name<latin::Comparison>(f, at + 2, where)};
}

// NOTE: Built field by field: as one braced return, GCC 16.2 inlines it into
//  init() and warns maybe-uninitialized.
template <std::size_t max>
Numeral numeral(const Fields<max>& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kNumeralFields, latin::Part::NUM, where);
  Numeral numeral{};
  numeral.declension = number<latin::Declension>(f, at, where);
  numeral.declensionVariant = number<latin::Variant>(f, at + 1, where);
  numeral.numeralSort = name<latin::NumeralSort>(f, at + 2, where);
  numeral.numeralValue = number<latin::NumeralValue>(f, at + 3, where);
  return numeral;
}

template <std::size_t max>
Packon packon(const Fields<max>& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kPackonFields, latin::Part::PACK, where);
  return {number<latin::Declension>(f, at, where),
          number<latin::Variant>(f, at + 1, where),
          name<latin::PackonKind>(f, at + 2, where)};
}

template <std::size_t max>
Adverb adverb(const Fields<max>& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kAdverbFields, latin::Part::ADV, where);
  return {name<latin::Comparison>(f, at, where)};
}

template <std::size_t max>
Preposition preposition(const Fields<max>& f, std::size_t at,
                        std::string_view where) {
  expect(f, at, scheme::kPrepositionFields, latin::Part::PREP, where);
  return {name<latin::Case>(f, at, where)};
}

} // namespace lexeme

namespace dict {

namespace scheme = dictlineScheme;
using namespace latin;

template <std::size_t at, std::size_t width>
std::string_view slice(std::string_view text) {
  return text.substr(at, width);
}

template <std::size_t which> std::string_view stemSlice(std::string_view text) {
  return slice<scheme::kStemAt + which * scheme::kStemWidth,
               scheme::kStemWidth>(text);
}

word::Stem stem(std::string_view slice, std::string_view where) {
  const std::string_view unpad = trimmed(slice, scheme::kFieldSeparator);
  if (unpad.empty())
    return {};
  if (unpad == word::kAbsentStem)
    return word::kAbsentStem;
  if (unpad.contains(scheme::kFieldSeparator))
    error::fatal(std::string{where} + ": stem has a space inside its text");
  return unpad;
}

Parsed<Entry> read(latin::Part part, const Fields<scheme::kGrammarMaxFields>& f,
                   std::string_view where) {
  switch (part) {
  case latin::Part::N:
    return {lexeme::noun(f, 0, where), scheme::kNounFields};
  case latin::Part::PRON:
    return {lexeme::pronoun(f, 0, where), scheme::kPronounFields};
  case latin::Part::V:
    return {lexeme::verb(f, 0, where), scheme::kVerbFields};
  case latin::Part::ADJ:
    return {lexeme::adjective(f, 0, where), scheme::kAdjectiveFields};
  case latin::Part::NUM:
    return {lexeme::numeral(f, 0, where), scheme::kNumeralFields};
  case latin::Part::PACK:
    return {lexeme::packon(f, 0, where), scheme::kPackonFields};
  case latin::Part::ADV:
    return {lexeme::adverb(f, 0, where), scheme::kAdverbFields};
  case latin::Part::PREP:
    return {lexeme::preposition(f, 0, where), scheme::kPrepositionFields};
  case latin::Part::CONJ:
    return {Conjunction{}, scheme::kConjunctionFields};
  case latin::Part::INTERJ:
    return {Interjection{}, scheme::kInterjectionFields};
  case latin::Part::SUPINE:
  case latin::Part::VPAR:
  case latin::Part::NONE:
  case latin::Part::X:
    break;
  }
  error::fatal(std::string{where} + ": part is not a DICTLINE part");
}

Entry grammar(latin::Part part, std::string_view slice,
              std::string_view where) {
  const auto f = split<scheme::kGrammarMaxFields>(
      slice, std::string_view{&scheme::kFieldSeparator, 1}, where);
  const Parsed<Entry> parsed = read(part, f, where);
  done(f, parsed.fields, where);
  return parsed.value;
}

Labels labels(std::string_view slice, std::string_view where) {
  for (std::size_t at = 0; at < slice.size(); at += scheme::kLabelStride)
    if (slice[at] != scheme::kFieldSeparator)
      error::fatal(std::string{where} + ": label separator at offset " +
                   std::to_string(at) + " is not a space");
  return {
      .age = letter<latin::Age>(slice[scheme::kAgeAt], "age", where),
      .area = letter<latin::Area>(slice[scheme::kAreaAt], "area", where),
      .geography = letter<latin::Geography>(slice[scheme::kGeographyAt],
                                            "geography", where),
      .frequency = letter<latin::Frequency>(slice[scheme::kFrequencyAt],
                                            "frequency", where),
      .source =
          letter<latin::Source>(slice[scheme::kSourceAt], "source", where),
  };
}

std::string_view packonOf(std::string_view senses) {
  constexpr std::string_view open{"(w/-"};
  if (!senses.starts_with(open))
    return {};
  const std::size_t close = senses.find(')', open.size());
  if (close == std::string_view::npos)
    return {};
  return senses.substr(open.size(), close - open.size());
}

DictlineEntry parse(std::string_view text, std::string_view where) {
  if (text.size() < scheme::kSensesAt + scheme::kSensesMinLength)
    error::fatal(std::string{where} + ": record is " +
                 std::to_string(text.size()) +
                 " bytes, the format needs at least " +
                 std::to_string(scheme::kSensesAt + scheme::kSensesMinLength));
  DictlineEntry entry{
      .stem1 = stem(stemSlice<0>(text), where),
      .stem2 = stem(stemSlice<1>(text), where),
      .stem3 = stem(stemSlice<2>(text), where),
      .stem4 = stem(stemSlice<3>(text), where),
      .grammar = grammar(
          part(scheme::kParts, "a DICTLINE",
               trimmed(slice<scheme::kPartAt, scheme::kPartWidth>(text),
                       scheme::kFieldSeparator),
               where),
          slice<scheme::kGrammarAt, scheme::kGrammarWidth>(text), where),
      .labels =
          labels(slice<scheme::kLabelsAt, scheme::kLabelsWidth>(text), where),
      .senses = printable(text.substr(scheme::kSensesAt),
                          scheme::kSensesMinLength, scheme::kSensesLowestByte,
                          scheme::kSensesHighestByte, "senses", where),
      .packon = {},
  };
  if (std::holds_alternative<Packon>(entry.grammar))
    entry.packon = packonOf(entry.senses);
  return entry;
}

} // namespace dict

namespace inflect {

namespace scheme = inflectsScheme;
using Line = Fields<scheme::kMaxFields>;
using Grammar = Parsed<latin::Inflection>;
using namespace latin::inflected;

struct Ending {
  latin::StemKey stemKey;
  latin::CharacterCount characterCount;
  std::string_view text;
  std::size_t fields;
};

Grammar noun(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kNounFields, latin::Part::N, where);
  return {Noun{number<latin::Declension>(f, at, where),
               number<latin::Variant>(f, at + 1, where),
               name<latin::Case>(f, at + 2, where),
               name<latin::Number>(f, at + 3, where),
               name<latin::Gender>(f, at + 4, where)},
          scheme::kNounFields};
}

Grammar pronoun(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kPronounFields, latin::Part::PRON, where);
  return {Pronoun{number<latin::Declension>(f, at, where),
                  number<latin::Variant>(f, at + 1, where),
                  name<latin::Case>(f, at + 2, where),
                  name<latin::Number>(f, at + 3, where),
                  name<latin::Gender>(f, at + 4, where)},
          scheme::kPronounFields};
}

Grammar adjective(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kAdjectiveFields, latin::Part::ADJ, where);
  return {Adjective{number<latin::Declension>(f, at, where),
                    number<latin::Variant>(f, at + 1, where),
                    name<latin::Case>(f, at + 2, where),
                    name<latin::Number>(f, at + 3, where),
                    name<latin::Gender>(f, at + 4, where),
                    name<latin::Comparison>(f, at + 5, where)},
          scheme::kAdjectiveFields};
}

Grammar numeral(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kNumeralFields, latin::Part::NUM, where);
  return {Numeral{number<latin::Declension>(f, at, where),
                  number<latin::Variant>(f, at + 1, where),
                  name<latin::Case>(f, at + 2, where),
                  name<latin::Number>(f, at + 3, where),
                  name<latin::Gender>(f, at + 4, where),
                  name<latin::NumeralSort>(f, at + 5, where)},
          scheme::kNumeralFields};
}

Grammar verb(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kVerbFields, latin::Part::V, where);
  return {Verb{number<latin::Conjugation>(f, at, where),
               number<latin::Variant>(f, at + 1, where),
               name<latin::Tense>(f, at + 2, where),
               name<latin::Voice>(f, at + 3, where),
               name<latin::Mood>(f, at + 4, where),
               number<latin::Person>(f, at + 5, where),
               name<latin::Number>(f, at + 6, where)},
          scheme::kVerbFields};
}

Grammar participle(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kParticipleFields, latin::Part::VPAR, where);
  return {Participle{number<latin::Conjugation>(f, at, where),
                     number<latin::Variant>(f, at + 1, where),
                     name<latin::Case>(f, at + 2, where),
                     name<latin::Number>(f, at + 3, where),
                     name<latin::Gender>(f, at + 4, where),
                     name<latin::Tense>(f, at + 5, where),
                     name<latin::Voice>(f, at + 6, where),
                     name<latin::Mood>(f, at + 7, where)},
          scheme::kParticipleFields};
}

Grammar supine(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kSupineFields, latin::Part::SUPINE, where);
  return {Supine{number<latin::Conjugation>(f, at, where),
                 number<latin::Variant>(f, at + 1, where),
                 name<latin::Case>(f, at + 2, where),
                 name<latin::Number>(f, at + 3, where),
                 name<latin::Gender>(f, at + 4, where)},
          scheme::kSupineFields};
}

Grammar adverb(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kAdverbFields, latin::Part::ADV, where);
  return {Adverb{name<latin::Comparison>(f, at, where)}, scheme::kAdverbFields};
}

Grammar preposition(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kPrepositionFields, latin::Part::PREP, where);
  return {Preposition{name<latin::Case>(f, at, where)},
          scheme::kPrepositionFields};
}

Grammar grammar(latin::Part part, const Line& f, std::size_t at,
                std::string_view where) {
  switch (part) {
  case latin::Part::N:
    return noun(f, at, where);
  case latin::Part::PRON:
    return pronoun(f, at, where);
  case latin::Part::ADJ:
    return adjective(f, at, where);
  case latin::Part::NUM:
    return numeral(f, at, where);
  case latin::Part::V:
    return verb(f, at, where);
  case latin::Part::VPAR:
    return participle(f, at, where);
  case latin::Part::SUPINE:
    return supine(f, at, where);
  case latin::Part::ADV:
    return adverb(f, at, where);
  case latin::Part::PREP:
    return preposition(f, at, where);
  case latin::Part::CONJ:
    return {Conjunction{}, scheme::kConjunctionFields};
  case latin::Part::INTERJ:
    return {Interjection{}, scheme::kInterjectionFields};
  case latin::Part::PACK:
  case latin::Part::NONE:
  case latin::Part::X:
    break;
  }
  error::fatal(std::string{where} + ": part is not an INFLECTS part");
}

Ending ending(const Line& f, std::size_t at, std::string_view where) {
  if (f.count < at + scheme::kEndingFields)
    error::fatal(std::string{where} +
                 ": record ends before stem key and count");
  Ending ending{.stemKey = number<latin::StemKey>(f, at, where),
                .characterCount =
                    number<latin::CharacterCount>(f, at + 1, where),
                .text = {},
                .fields = scheme::kEndingFields};
  if (ending.characterCount.value == 0)
    return ending;
  if (f.count < at + scheme::kEndingFields + 1)
    error::fatal(std::string{where} + ": count is not 0 but no ending follows");
  ending.text = f.at[at + scheme::kEndingFields];
  ending.fields = scheme::kEndingFields + 1;
  for (const char c : ending.text)
    if (c < 'a' || c > 'z')
      error::fatal(std::string{where} + ": ending '" +
                   std::string{ending.text} + "' is not lowercase letters");
  return ending;
}

InflectsEntry parse(std::string_view text, std::string_view where) {
  const Line f = split<scheme::kMaxFields>(
      text.substr(0, text.find(scheme::kCommentMarker)), scheme::kWhitespace,
      where);
  if (f.count == 0)
    error::fatal(std::string{where} + ": record has no fields");

  std::size_t at = 1;
  const Grammar parsed = grammar(
      part(scheme::kParts, "an INFLECTS", f.at[0], where), f, at, where);
  at += parsed.fields;
  const Ending end = ending(f, at, where);
  at += end.fields;
  labelsFollow(f, at, scheme::kLabelCount, "ending", where);
  return {.grammar = parsed.value,
          .stemKey = end.stemKey,
          .characterCount = end.characterCount,
          .ending = end.text,
          .age = letter<latin::Age>(f.at[at], "age", where),
          .frequency =
              letter<latin::Frequency>(f.at[at + 1], "frequency", where)};
}

} // namespace inflect

namespace unique {

namespace scheme = uniquesScheme;
using Line = Fields<scheme::kMaxFields>;
using namespace latin::inflected;
static_assert(std::is_same_v<Line, inflect::Line>,
              "UNIQUES reads its grammar with INFLECTS' readers");

struct Grammar {
  latin::Entry word;
  latin::Inflection form;
  std::size_t fields;
};

Grammar noun(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kNounFields, latin::Part::N, where);
  const auto form = std::get<Noun>(inflect::noun(f, at, where).value);
  return {latin::Noun{form.declension, form.declensionVariant, form.gender,
                      name<latin::NounKind>(f, at + 5, where)},
          form, scheme::kNounFields};
}

Grammar pronoun(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kPronounFields, latin::Part::PRON, where);
  const auto form = std::get<Pronoun>(inflect::pronoun(f, at, where).value);
  return {latin::Pronoun{form.declension, form.declensionVariant,
                         name<latin::PronounKind>(f, at + 5, where)},
          form, scheme::kPronounFields};
}

Grammar adjective(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kAdjectiveFields, latin::Part::ADJ, where);
  const auto form = std::get<Adjective>(inflect::adjective(f, at, where).value);
  return {latin::Adjective{form.declension, form.declensionVariant,
                           form.comparison},
          form, scheme::kAdjectiveFields};
}

Grammar verb(const Line& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kVerbFields, latin::Part::V, where);
  const auto form = std::get<Verb>(inflect::verb(f, at, where).value);
  return {latin::Verb{form.conjugation, form.conjugationVariant,
                      name<latin::VerbKind>(f, at + 7, where)},
          form, scheme::kVerbFields};
}

Grammar grammar(latin::Part part, const Line& f, std::size_t at,
                std::string_view where) {
  switch (part) {
  case latin::Part::N:
    return noun(f, at, where);
  case latin::Part::PRON:
    return pronoun(f, at, where);
  case latin::Part::ADJ:
    return adjective(f, at, where);
  case latin::Part::V:
    return verb(f, at, where);
  case latin::Part::ADV:
  case latin::Part::PREP:
  case latin::Part::NUM:
  case latin::Part::CONJ:
  case latin::Part::INTERJ:
  case latin::Part::PACK:
  case latin::Part::SUPINE:
  case latin::Part::VPAR:
  case latin::Part::NONE:
  case latin::Part::X:
    break;
  }
  error::fatal(std::string{where} + ": part is not a UNIQUES part");
}

std::string_view form(std::string_view word, std::string_view where) {
  const std::string_view form = trimmed(word, scheme::kTrailingPadding);
  if (form.empty())
    error::fatal(std::string{where} + ": word line is blank");
  letters(form, "word", where);
  return form;
}

UniquesEntry parse(const source::UniquesLines& lines, std::string_view where) {
  const Line f = split<scheme::kMaxFields>(
      lines[std::to_underlying(scheme::Line::attributes)], scheme::kWhitespace,
      where);
  if (f.count == 0)
    error::fatal(std::string{where} + ": attribute line has no fields");
  std::size_t at = 1;
  const Grammar parsed =
      grammar(part(scheme::kParts, "a UNIQUES", f.at[0], where), f, at, where);
  at += parsed.fields;
  labelsFollow(f, at, scheme::kLabelCount, "grammar", where);
  return {
      .form = form(lines[std::to_underlying(scheme::Line::word)], where),
      .grammar = parsed.word,
      .inflection = parsed.form,
      .labels = {.age = letter<latin::Age>(f.at[at], "age", where),
                 .area = letter<latin::Area>(f.at[at + 1], "area", where),
                 .geography =
                     letter<latin::Geography>(f.at[at + 2], "geography", where),
                 .frequency =
                     letter<latin::Frequency>(f.at[at + 3], "frequency", where),
                 .source =
                     letter<latin::Source>(f.at[at + 4], "source", where)},
      .senses = printable(lines[std::to_underlying(scheme::Line::meaning)],
                          scheme::kSensesMinLength, scheme::kSensesLowestByte,
                          scheme::kSensesHighestByte, "meaning", where),
  };
}

} // namespace unique

namespace affix {

namespace scheme = addonsScheme;
using Line = Fields<scheme::kGrammarMaxFields>;
using namespace latin;

enum class Kind { prefix, suffix, tackon };

// NOTE: A missing connecting letter is '\0'.
struct Fix {
  Kind kind;
  std::string_view text;
  char connect;
};

Kind kind(std::string_view tag, std::string_view where) {
  if (tag == scheme::kPrefixTag)
    return Kind::prefix;
  if (tag == scheme::kSuffixTag)
    return Kind::suffix;
  if (tag == scheme::kTackonTag)
    return Kind::tackon;
  error::fatal(std::string{where} + ": not an ADDONS kind: '" +
               std::string{tag} + "'");
}

// NOTE: The fix line may carry a trailing comment and the meaning line may
//  not, so the cut happens here, not in the source island.
Fix fix(std::string_view line, std::string_view where) {
  const auto f = split<scheme::kFixMaxFields>(
      line.substr(0, line.find(scheme::kCommentMarker)), scheme::kWhitespace,
      where);
  if (f.count < scheme::kFixMinFields)
    error::fatal(std::string{where} + ": fix line needs a kind and a fix");
  Fix fix{.kind = kind(f.at[0], where), .text = f.at[1], .connect = '\0'};
  letters(fix.text, "fix", where);
  if (f.count == scheme::kFixMaxFields) {
    if (fix.kind == Kind::tackon)
      error::fatal(std::string{where} + ": TACKON has no connect, found '" +
                   std::string{f.at[2]} + "'");
    if (f.at[2].size() != 1)
      error::fatal(std::string{where} + ": connect '" + std::string{f.at[2]} +
                   "' is not one letter");
    letters(f.at[2], "connect", where);
    fix.connect = f.at[2][0];
  }
  return fix;
}

latin::Part part(const Line& f, std::size_t i, std::string_view where) {
  return ::part(scheme::kParts, "an ADDONS", f.at[i], where);
}

Parsed<addon::Target> target(latin::Part part, const Line& f, std::size_t at,
                             std::string_view where) {
  switch (part) {
  case latin::Part::X:
    return {addon::Any{}, scheme::kAnyFields};
  case latin::Part::N:
    return {lexeme::noun(f, at, where), dictlineScheme::kNounFields};
  case latin::Part::PRON:
    return {lexeme::pronoun(f, at, where), dictlineScheme::kPronounFields};
  case latin::Part::ADJ:
    return {lexeme::adjective(f, at, where), dictlineScheme::kAdjectiveFields};
  case latin::Part::NUM:
    return {lexeme::numeral(f, at, where), dictlineScheme::kNumeralFields};
  case latin::Part::V:
    return {lexeme::verb(f, at, where), dictlineScheme::kVerbFields};
  case latin::Part::ADV:
    return {lexeme::adverb(f, at, where), dictlineScheme::kAdverbFields};
  case latin::Part::PACK:
    return {lexeme::packon(f, at, where), dictlineScheme::kPackonFields};
  case latin::Part::PREP:
  case latin::Part::CONJ:
  case latin::Part::INTERJ:
  case latin::Part::SUPINE:
  case latin::Part::VPAR:
  case latin::Part::NONE:
    break;
  }
  error::fatal(std::string{where} + ": no addon targets part " +
               std::string{util::enumToSv(part)});
}

Addon grammar(const Fix& fix, std::string_view line, std::string_view where) {
  const Line f =
      split<scheme::kGrammarMaxFields>(line, scheme::kWhitespace, where);
  switch (fix.kind) {
  case Kind::prefix: {
    if (f.count != scheme::kPrefixFields)
      error::fatal(std::string{where} + ": PREFIX grammar states " +
                   std::to_string(scheme::kPrefixFields) +
                   " fields, line has " + std::to_string(f.count));
    return addon::Prefix{fix.text, fix.connect, part(f, 0, where),
                         part(f, 1, where)};
  }
  case Kind::tackon: {
    if (f.count < scheme::kTackonLeadFields)
      error::fatal(std::string{where} + ": TACKON grammar is empty");
    const latin::Part to = part(f, 0, where);
    const auto read = target(to, f, scheme::kTackonLeadFields, where);
    done(f, scheme::kTackonLeadFields + read.fields, where);
    return addon::Tackon{fix.text, to, read.value};
  }
  case Kind::suffix: {
    if (f.count < scheme::kSuffixLeadFields + scheme::kSuffixTailFields)
      error::fatal(std::string{where} +
                   ": SUFFIX grammar needs from, key, target, key");
    const latin::Part from = part(f, 0, where);
    const latin::StemKey fromKey = number<latin::StemKey>(f, 1, where);
    const latin::Part to = part(f, 2, where);
    const auto read = target(to, f, scheme::kSuffixLeadFields, where);
    const std::size_t at = scheme::kSuffixLeadFields + read.fields;
    if (f.count != at + scheme::kSuffixTailFields)
      error::fatal(std::string{where} +
                   ": SUFFIX needs one key after the target grammar");
    return addon::Suffix{fix.text,
                         fix.connect,
                         from,
                         fromKey,
                         to,
                         read.value,
                         number<latin::StemKey>(f, at, where)};
  }
  }
  error::fatal(std::string{where} + ": unknown fix kind");
}

AddonKind kindOf(const Addon& addon, std::string_view meaning) {
  if (const auto* prefix = std::get_if<addon::Prefix>(&addon))
    return prefix->to == Part::PACK ? AddonKind::Tickon : AddonKind::Prefix;
  if (std::holds_alternative<addon::Suffix>(addon))
    return AddonKind::Suffix;
  const auto* packon =
      std::get_if<Packon>(&std::get<addon::Tackon>(addon).grammar);
  return packon != nullptr && latin::isPackon(packon->declension, meaning)
             ? AddonKind::Packon
             : AddonKind::Tackon;
}

AddonsEntry parse(const source::AddonsLines& lines, std::string_view where) {
  const Addon addon =
      grammar(fix(lines[std::to_underlying(scheme::Line::fix)], where),
              lines[std::to_underlying(scheme::Line::partEntry)], where);
  const std::string_view meaning =
      printable(trimmed(lines[std::to_underlying(scheme::Line::meaning)],
                        scheme::kTrailingPadding),
                scheme::kMeaningMinLength, scheme::kMeaningLowestByte,
                scheme::kMeaningHighestByte, "meaning", where);
  return {.addon = addon, .meaning = meaning, .kind = kindOf(addon, meaning)};
}

std::string_view fixOf(const Addon& addon) {
  return std::visit([](const auto& a) { return a.fix; }, addon);
}

} // namespace affix

std::string partsLine(std::span<const std::string_view> parts,
                      const std::map<std::string_view, std::size_t>& counted) {
  std::string line = "  parts:";
  for (const std::string_view part : parts) {
    const auto found = counted.find(part);
    line += " " + std::string{part} + " " +
            std::to_string(found == counted.end() ? 0 : found->second);
  }
  return line;
}

void report() {
  const auto dictlineEntries =
      std::span{dictline}.first(dictlineScheme::kEntriesPerFile);
  std::println("word dictline: {} records", dictlineEntries.size());
  const auto stems = [dictlineEntries](word::Stem DictlineEntry::* column,
                                       std::string_view name) {
    std::size_t text = 0, absent = 0, empty = 0;
    for (const DictlineEntry& entry : dictlineEntries)
      switch (word::stemState(entry.*column)) {
      case word::StemState::text:
        ++text;
        break;
      case word::StemState::absent:
        ++absent;
        break;
      case word::StemState::empty:
        ++empty;
        break;
      }
    std::println("  {}: text {} zzz {} blank {}", name, text, absent, empty);
  };
  stems(&DictlineEntry::stem1, "stem1");
  stems(&DictlineEntry::stem2, "stem2");
  stems(&DictlineEntry::stem3, "stem3");
  stems(&DictlineEntry::stem4, "stem4");
  static_assert(dictlineScheme::kParts.size() ==
                std::variant_size_v<latin::Entry>);
  std::map<std::string_view, std::size_t> dictlineParts;
  for (const DictlineEntry& entry : dictlineEntries)
    ++dictlineParts[dictlineScheme::kParts[entry.grammar.index()]];
  std::println("{}", partsLine(dictlineScheme::kParts, dictlineParts));

  static_assert(inflectsScheme::kParts.size() ==
                std::variant_size_v<latin::Inflection>);
  std::map<std::string_view, std::size_t> inflectsParts;
  std::size_t present = 0, absent = 0, disagree = 0;
  std::map<std::size_t, std::size_t> count, key;
  std::map<latin::Age, std::size_t> age;
  std::map<latin::Frequency, std::size_t> frequency;
  for (std::size_t i = 0; i < inflectsScheme::kEntriesPerFile; ++i) {
    const InflectsEntry& entry = inflects[i];
    if (entry.characterCount.value > entry.ending.size())
      error::fatal("inflects record " + std::to_string(i + 1) + ": declares " +
                   std::to_string(entry.characterCount.value) +
                   " characters, ending holds " +
                   std::to_string(entry.ending.size()));
    ++inflectsParts[inflectsScheme::kParts[entry.grammar.index()]];
    entry.ending.empty() ? ++absent : ++present;
    if (!entry.ending.empty() &&
        entry.ending.size() != entry.characterCount.value)
      ++disagree;
    ++count[entry.characterCount.value];
    ++key[entry.stemKey.value];
    ++age[entry.age];
    ++frequency[entry.frequency];
  }
  std::println("word inflects: {} records", inflectsScheme::kEntriesPerFile);
  std::println("{}", partsLine(inflectsScheme::kParts, inflectsParts));
  std::println("  ending: present {} absent {}; count != length on {} records",
               present, absent, disagree);
  std::println("  character count:{}", counts(count));
  std::println("  stem key:{}", counts(key));
  std::println("  age:{}", counts(age));
  std::println("  frequency:{}", counts(frequency));

  std::map<std::string_view, std::size_t> uniquesParts;
  std::size_t lowercaseKinds = 0;
  std::map<latin::Age, std::size_t> uniqueAge;
  std::map<latin::Area, std::size_t> area;
  std::map<latin::Geography, std::size_t> geography;
  std::map<latin::Frequency, std::size_t> uniqueFrequency;
  std::map<latin::Source, std::size_t> attestation;
  for (const UniquesEntry& entry : uniques) {
    ++uniquesParts[inflectsScheme::kParts[entry.inflection.index()]];
    if (const auto* noun = std::get_if<latin::Noun>(&entry.grammar))
      if (noun->nounKind == latin::NounKind::p ||
          noun->nounKind == latin::NounKind::t ||
          noun->nounKind == latin::NounKind::w ||
          noun->nounKind == latin::NounKind::x)
        ++lowercaseKinds;
    ++uniqueAge[entry.labels.age];
    ++area[entry.labels.area];
    ++geography[entry.labels.geography];
    ++uniqueFrequency[entry.labels.frequency];
    ++attestation[entry.labels.source];
  }
  std::println("word uniques: {} entries", uniques.size());
  std::println("{}; lowercase noun kinds {}",
               partsLine(uniquesScheme::kParts, uniquesParts), lowercaseKinds);
  std::println("  age:{}", counts(uniqueAge));
  std::println("  area:{}", counts(area));
  std::println("  geography:{}", counts(geography));
  std::println("  frequency:{}", counts(uniqueFrequency));
  std::println("  source:{}", counts(attestation));

  std::size_t prefixes = 0, suffixes = 0, tackons = 0, connects = 0;
  std::map<latin::Part, std::size_t> prefixTo, suffixFrom, suffixTarget,
      tackonTarget;
  std::map<std::size_t, std::size_t> fromKey, toKey, fixLength;
  for (const AddonsEntry& entry : addons) {
    const latin::Addon& addon = entry.addon;
    ++fixLength[affix::fixOf(addon).size()];
    if (const auto* p = std::get_if<latin::addon::Prefix>(&addon)) {
      ++prefixes;
      if (p->connect)
        ++connects;
      ++prefixTo[p->to];
    } else if (const auto* s = std::get_if<latin::addon::Suffix>(&addon)) {
      ++suffixes;
      if (s->connect)
        ++connects;
      ++suffixFrom[s->from];
      ++suffixTarget[s->target];
      ++fromKey[s->fromKey.value];
      ++toKey[s->toKey.value];
    } else if (const auto* t = std::get_if<latin::addon::Tackon>(&addon)) {
      ++tackons;
      ++tackonTarget[t->target];
    }
  }
  std::println("word addons: {} entries", addons.size());
  std::println("  kinds: PREFIX {} SUFFIX {} TACKON {}; connect present {}",
               prefixes, suffixes, tackons, connects);
  std::println("  PREFIX to:{}", counts(prefixTo));
  std::println("  SUFFIX from:{}", counts(suffixFrom));
  std::println("  SUFFIX target:{}", counts(suffixTarget));
  std::println("  SUFFIX fromKey:{}  toKey:{}", counts(fromKey), counts(toKey));
  std::println("  TACKON target:{}", counts(tackonTarget));
  std::println("  fix length:{}", counts(fixLength));
}

} // namespace

void word::init() {
  for (std::size_t i = 0; i < dictlineScheme::kEntriesPerFile; ++i) {
    const source::DictlineIndex entry{i};
    dictline[i] = dict::parse(source::dictline(entry),
                              where(main_config::kSourceDictlinePath, entry));
  }
  for (std::size_t i = 0; i < inflectsScheme::kEntriesPerFile; ++i) {
    const source::InflectsIndex entry{i};
    inflects[i] =
        inflect::parse(source::inflects(entry),
                       where(main_config::kSourceInflectsPath, entry));
  }
  for (std::size_t i = 0; i < uniques.size(); ++i) {
    const source::UniquesIndex entry{i};
    uniques[i] = unique::parse(source::uniques(entry),
                               where(main_config::kSourceUniquesPath, entry));
  }
  for (std::size_t i = 0; i < addons.size(); ++i) {
    const source::AddonsIndex entry{i};
    addons[i] = affix::parse(source::addons(entry),
                             where(main_config::kSourceAddonsPath, entry));
  }

  // NOTE: The second column is present and empty, not absent.
  dictline[kEsse.value] = {
      .stem1 = "s",
      .stem2 = "",
      .stem3 = "fu",
      .stem4 = "fut",
      .grammar = latin::Verb{latin::Conjugation{5}, latin::Variant{1},
                             latin::VerbKind::TO_BE},
      .labels = {.age = latin::Age::X,
                 .area = latin::Area::X,
                 .geography = latin::Geography::X,
                 .frequency = latin::Frequency::A,
                 .source = latin::Source::X},
      .senses = "be; exist; (also used to form verb perfect passive "
                "tenses) with NOM PERF PPL",
      .packon = {},
  };
  // NOTE: Fix_Adverb's two readings need an inflection identity; these
  //  rows are it, and the join never crosses them.
  inflects[kAdverbPositive.value] = {
      .grammar = latin::inflected::Adverb{latin::Comparison::POS},
      .stemKey = latin::StemKey{0},
      .characterCount = latin::CharacterCount{1},
      .ending = "e",
      .age = latin::Age::X,
      .frequency = latin::Frequency::B};
  inflects[kAdverbSuperlative.value] = {
      .grammar = latin::inflected::Adverb{latin::Comparison::SUPER},
      .stemKey = latin::StemKey{0},
      .characterCount = latin::CharacterCount{2},
      .ending = "me",
      .age = latin::Age::X,
      .frequency = latin::Frequency::B};
  report();
}

word::Stem word::Dictline::stem1() const { return dictline[entry.value].stem1; }
word::Stem word::Dictline::stem2() const { return dictline[entry.value].stem2; }
word::Stem word::Dictline::stem3() const { return dictline[entry.value].stem3; }
word::Stem word::Dictline::stem4() const { return dictline[entry.value].stem4; }
latin::Entry word::Dictline::grammar() const {
  return dictline[entry.value].grammar;
}
latin::Age word::Dictline::age() const {
  return dictline[entry.value].labels.age;
}
latin::Area word::Dictline::area() const {
  return dictline[entry.value].labels.area;
}
latin::Geography word::Dictline::geography() const {
  return dictline[entry.value].labels.geography;
}
latin::Frequency word::Dictline::frequency() const {
  return dictline[entry.value].labels.frequency;
}
latin::Source word::Dictline::source() const {
  return dictline[entry.value].labels.source;
}
std::string_view word::Dictline::senses() const {
  return dictline[entry.value].senses;
}
std::string_view word::Dictline::packon() const {
  return dictline[entry.value].packon;
}

latin::Inflection word::Inflects::grammar() const {
  return inflects[entry.value].grammar;
}
latin::StemKey word::Inflects::stemKey() const {
  return inflects[entry.value].stemKey;
}
latin::CharacterCount word::Inflects::characterCount() const {
  return inflects[entry.value].characterCount;
}
std::string_view word::Inflects::ending() const {
  return inflects[entry.value].ending;
}
latin::Age word::Inflects::age() const { return inflects[entry.value].age; }
latin::Frequency word::Inflects::frequency() const {
  return inflects[entry.value].frequency;
}

std::string_view word::Uniques::form() const {
  return uniques[entry.value].form;
}
latin::Entry word::Uniques::grammar() const {
  return uniques[entry.value].grammar;
}
latin::Inflection word::Uniques::inflection() const {
  return uniques[entry.value].inflection;
}
latin::Age word::Uniques::age() const {
  return uniques[entry.value].labels.age;
}
latin::Area word::Uniques::area() const {
  return uniques[entry.value].labels.area;
}
latin::Geography word::Uniques::geography() const {
  return uniques[entry.value].labels.geography;
}
latin::Frequency word::Uniques::frequency() const {
  return uniques[entry.value].labels.frequency;
}
latin::Source word::Uniques::source() const {
  return uniques[entry.value].labels.source;
}
std::string_view word::Uniques::senses() const {
  return uniques[entry.value].senses;
}

latin::Addon word::Addons::addon() const { return addons[entry.value].addon; }
std::string_view word::Addons::meaning() const {
  return addons[entry.value].meaning;
}
latin::AddonKind word::Addons::kind() const { return addons[entry.value].kind; }
