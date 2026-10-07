#include "word.hpp"

#include "error/error.hpp"
#include "source/schemes.hpp"
#include "util/reflect_util.hpp"
#include "word/field.hpp"
#include "word/split.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <meta>
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
using word::internal::byPart;
using word::internal::Fields;
using word::internal::fill;
using word::internal::name;
using word::internal::number;
using word::internal::Parsed;
using word::internal::split;
using word::internal::tag;

using word::AddonsEntry;
using word::DictlineEntry;
using word::InflectsEntry;
using word::UniquesEntry;

std::array<DictlineEntry, word::kDictlineWords> dictline;
std::array<InflectsEntry, word::kInflectsWords> inflects;
std::array<UniquesEntry, uniquesScheme::kEntriesPerFile> uniques;
std::array<AddonsEntry, addonsScheme::kEntriesPerFile> addons;

template <typename Index> std::string where(std::string_view path, Index entry) {
  return std::string{path} + ":" + std::to_string(source::lineNumber(entry));
}

template <typename E> E letter(char c, std::string_view name, std::string_view where) {
  if (const auto value = util::trySvToEnum<E>(std::string_view{&c, 1}))
    return *value;
  error::invalid(std::string{name} + " '" + c + "'", where);
}

template <typename E> E letter(std::string_view field, std::string_view name, std::string_view where) {
  if (field.size() != 1)
    error::invalid(std::string{name} + " '" + std::string{field} + "'", where);
  return letter<E>(field.front(), name, where);
}

latin::Part part(std::span<const latin::Part> parts, std::string_view file, std::string_view token,
                 std::string_view where) {
  for (const latin::Part part : parts)
    if (util::enumToSv(part) == token)
      return part;
  error::invalid(std::string{file} + " part '" + std::string{token} + "'", where);
}

template <std::size_t max>
void expect(const Fields<max>& f, std::size_t at, std::size_t arity, latin::Part part, std::string_view where) {
  if (f.count < at + arity)
    error::invalid(std::string{util::enumToSv(part)} + " grammar of " + std::to_string(f.count - at) + " fields (" +
                       std::to_string(arity) + " needed)",
                   where);
}

template <std::size_t max> void done(const Fields<max>& f, std::size_t at, std::string_view where) {
  if (f.count != at)
    error::invalid(std::to_string(f.count - at) + " fields after the grammar", where);
}

template <std::size_t max>
void labelsFollow(const Fields<max>& f, std::size_t at, std::size_t labels, std::string_view after,
                  std::string_view where) {
  if (f.count != at + labels)
    error::invalid(std::to_string(f.count - at) + " fields after the " + std::string{after} + " (" +
                       std::to_string(labels) + " expected)",
                   where);
}

std::string_view trimmed(std::string_view text, char padding) {
  const std::size_t end = text.find_last_not_of(padding);
  return end == std::string_view::npos ? std::string_view{} : text.substr(0, end + 1);
}

void letters(std::string_view text, std::string_view what, std::string_view where) {
  for (const char c : text)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
      error::invalid(std::string{what} + " '" + std::string{text} + "', not letters", where);
}

std::string_view printable(std::string_view text, std::string_view what, std::string_view where) {
  if (text.empty())
    error::invalid("empty " + std::string{what}, where);
  const auto low = std::ranges::find_if(text, [](char c) { return c < ' ' || c > '~'; });
  if (low != text.end())
    error::invalid(std::string{what} + " with a byte outside printable ASCII at offset " +
                       std::to_string(low - text.begin()),
                   where);
  return text;
}

namespace dict {

namespace scheme = dictlineScheme;
using namespace latin;

word::Stem stem(std::string_view slice, std::string_view where) {
  const std::string_view unpad = trimmed(slice, scheme::kFieldSeparator);
  if (unpad.empty())
    return std::string_view{};
  if (unpad == "zzz")
    return std::nullopt;
  if (unpad.contains(scheme::kFieldSeparator))
    error::invalid("stem with a space", where);
  return unpad;
}

Entry grammar(latin::Part part, std::string_view slice, std::string_view where) {
  const auto f = split<scheme::kGrammarMaxFields>(slice, std::string_view{&scheme::kFieldSeparator, 1}, where);
  const Parsed<Entry> parsed = byPart<Entry>(part, f, 0, where);
  done(f, parsed.fields, where);
  return parsed.value;
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

template <typename R> R columns(std::string_view text, std::string_view line, std::string_view where);

template <std::meta::info member> auto column(std::string_view slice, std::string_view line, std::string_view where) {
  using M = [:std::meta::type_of(member):];
  constexpr std::string_view name = std::meta::identifier_of(member);
  if constexpr (std::is_same_v<M, word::Stem>)
    return stem(slice, where);
  else if constexpr (std::is_same_v<M, std::string_view>)
    return printable(slice, name, where);
  else if constexpr (std::is_enum_v<M>)
    return letter<M>(slice.front(), name, where);
  else if constexpr (std::is_same_v<M, Entry>) {
    constexpr word::PartColumn named = *tag<word::PartColumn>(member);
    return grammar(
        part(scheme::kParts, "DICTLINE", trimmed(line.substr(named.at, named.width), scheme::kFieldSeparator), where),
        slice, where);
  } else
    return columns<M>(slice, line, where);
}

void blanks(std::string_view text, std::size_t from, std::size_t to, std::string_view where) {
  for (std::size_t at = from; at < to; ++at)
    if (text[at] != scheme::kFieldSeparator)
      error::invalid("separator at offset " + std::to_string(at) + ", not a space", where);
}

template <typename R> R columns(std::string_view text, std::string_view line, std::string_view where) {
  R out{};
  std::size_t covered = 0;
  template for (constexpr auto member : util::kMembers<R>) {
    constexpr std::optional<word::Column> at = tag<word::Column>(member);
    if constexpr (at.has_value()) {
      if constexpr (constexpr auto named = tag<word::PartColumn>(member)) {
        blanks(text, covered, named->at, where);
        covered = named->at + named->width;
      }
      blanks(text, covered, at->at, where);
      covered = at->width == 0 ? text.size() : at->at + at->width;
      out.[:member:] = column<member>(at->width == 0 ? text.substr(at->at) : text.substr(at->at, at->width), line,
                                      where);
    }
  }
  blanks(text, covered, text.size(), where);
  return out;
}

inline constexpr std::size_t kMinimumLength = tag<word::Column>(^^DictlineEntry::senses)->at + 1;

DictlineEntry parse(std::string_view text, std::string_view where) {
  if (text.size() < kMinimumLength)
    error::invalid(
        "record of " + std::to_string(text.size()) + " bytes (" + std::to_string(kMinimumLength) + " needed)", where);
  DictlineEntry entry = columns<DictlineEntry>(text, text, where);
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

template <typename R> bool nonZero(const R& out, std::string_view name) {
  bool result = false;
  template for (constexpr auto member : util::kMembers<R>) {
    constexpr std::string_view id = std::meta::identifier_of(member);
    if constexpr (requires { out.[:member:].value; })
      if (id == name)
        result = out.[:member:].value != 0;
  }
  return result;
}

template <typename R> R sequence(const Line& f, std::string_view where) {
  R out{};
  std::size_t i = 0;
  template for (constexpr auto member : util::kMembers<R>) {
    using M = [:std::meta::type_of(member):];
    constexpr std::string_view name = std::meta::identifier_of(member);
    bool present = true;
    if constexpr (constexpr auto when = tag<word::PresentIf>(member))
      present = nonZero(out, when->member);
    if (present) {
      if (i >= f.count)
        error::invalid("record ending before its " + std::string{name}, where);
      if constexpr (std::is_same_v<M, latin::Inflection>) {
        const Grammar parsed =
            byPart<latin::Inflection>(part(scheme::kParts, "INFLECTS", f.at[i], where), f, i + 1, where);
        out.[:member:] = parsed.value;
        i += 1 + parsed.fields;
      } else if constexpr (std::is_enum_v<M>) {
        out.[:member:] = letter<M>(f.at[i++], name, where);
      } else if constexpr (std::is_same_v<M, std::string_view>) {
        for (const char c : f.at[i])
          if (c < 'a' || c > 'z')
            error::invalid(std::string{name} + " '" + std::string{f.at[i]} + "', not lowercase letters", where);
        out.[:member:] = f.at[i++];
      } else {
        out.[:member:] = number<M>(f, i++, where);
      }
    }
  }
  if (f.count != i)
    error::invalid(std::to_string(f.count - i) + " extra fields", where);
  return out;
}

InflectsEntry parse(std::string_view text, std::string_view where) {
  const Line f =
      split<scheme::kMaxFields>(text.substr(0, text.find(scheme::kCommentMarker)), scheme::kWhitespace, where);
  if (f.count == 0)
    error::invalid("record without fields", where);
  const InflectsEntry entry = sequence<InflectsEntry>(f, where);
  if (entry.characterCount.value > entry.ending.size())
    error::invalid("character count " + std::to_string(entry.characterCount.value) + " for the ending '" +
                       std::string{entry.ending} + "'",
                   where);
  return entry;
}

} // namespace inflect

namespace unique {

namespace scheme = uniquesScheme;
using Line = inflect::Line;
using namespace latin::inflected;

struct Grammar {
  latin::Entry word;
  latin::Inflection form;
  std::size_t fields;
};

Grammar noun(const Line& f, std::size_t at, std::string_view where) {
  constexpr std::size_t form = util::kMembers<Noun>.size();
  expect(f, at, form + 1, latin::Part::N, where);
  const auto read = fill<Noun>(f, at, where);
  return {latin::Noun{read.declension, read.declensionVariant, read.gender, name<latin::NounKind>(f, at + form, where)},
          read, form + 1};
}

Grammar pronoun(const Line& f, std::size_t at, std::string_view where) {
  constexpr std::size_t form = util::kMembers<Pronoun>.size();
  expect(f, at, form + 1, latin::Part::PRON, where);
  const auto read = fill<Pronoun>(f, at, where);
  return {latin::Pronoun{read.declension, read.declensionVariant, name<latin::PronounKind>(f, at + form, where)}, read,
          form + 1};
}

Grammar adjective(const Line& f, std::size_t at, std::string_view where) {
  constexpr std::size_t form = util::kMembers<Adjective>.size();
  expect(f, at, form, latin::Part::ADJ, where);
  const auto read = fill<Adjective>(f, at, where);
  return {latin::Adjective{read.declension, read.declensionVariant, read.comparison}, read, form};
}

Grammar verb(const Line& f, std::size_t at, std::string_view where) {
  constexpr std::size_t form = util::kMembers<Verb>.size();
  expect(f, at, form + 1, latin::Part::V, where);
  const auto read = fill<Verb>(f, at, where);
  return {latin::Verb{read.conjugation, read.conjugationVariant, name<latin::VerbKind>(f, at + form, where)}, read,
          form + 1};
}

Grammar grammar(latin::Part part, const Line& f, std::size_t at, std::string_view where) {
  switch (part) {
  case latin::Part::N:
    return noun(f, at, where);
  case latin::Part::PRON:
    return pronoun(f, at, where);
  case latin::Part::ADJ:
    return adjective(f, at, where);
  case latin::Part::V:
    return verb(f, at, where);
  default:
    std::unreachable();
  }
}

std::string_view form(std::string_view word, std::string_view where) {
  const std::string_view form = trimmed(word, scheme::kTrailingPadding);
  if (form.empty())
    error::invalid("blank word line", where);
  letters(form, "word", where);
  return form;
}

UniquesEntry parse(const source::UniquesLines& lines, std::string_view where) {
  const Line f = split<inflectsScheme::kMaxFields>(lines[std::to_underlying(scheme::Line::attributes)],
                                                   scheme::kWhitespace, where);
  if (f.count == 0)
    error::invalid("attribute line without fields", where);
  std::size_t at = 1;
  const Grammar parsed = grammar(part(scheme::kParts, "UNIQUES", f.at[0], where), f, at, where);
  at += parsed.fields;
  labelsFollow(f, at, scheme::kLabelCount, "grammar", where);
  return {
      .form = form(lines[std::to_underlying(scheme::Line::word)], where),
      .grammar = parsed.word,
      .inflection = parsed.form,
      .labels = {.age = letter<latin::Age>(f.at[at], "age", where),
                 .area = letter<latin::Area>(f.at[at + 1], "area", where),
                 .geography = letter<latin::Geography>(f.at[at + 2], "geography", where),
                 .frequency = letter<latin::Frequency>(f.at[at + 3], "frequency", where),
                 .source = letter<latin::Source>(f.at[at + 4], "source", where)},
      .senses = printable(lines[std::to_underlying(scheme::Line::meaning)], "meaning", where),
  };
}

} // namespace unique

namespace affix {

namespace scheme = addonsScheme;
using Line = Fields<scheme::kGrammarMaxFields>;
using namespace latin;

enum class Kind { prefix, suffix, tackon };

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
  error::invalid("ADDONS kind '" + std::string{tag} + "'", where);
}

// NOTE: The fix line may carry a trailing comment and the meaning line may not, so the cut happens here, not in
//  source.cpp.
Fix fix(std::string_view line, std::string_view where) {
  const auto f =
      split<scheme::kFixMaxFields>(line.substr(0, line.find(scheme::kCommentMarker)), scheme::kWhitespace, where);
  if (f.count < scheme::kFixMinFields)
    error::invalid("fix line without a kind and a fix", where);
  Fix fix{.kind = kind(f.at[0], where), .text = f.at[1], .connect = '\0'};
  letters(fix.text, "fix", where);
  if (f.count == scheme::kFixMaxFields) {
    if (fix.kind == Kind::tackon)
      error::invalid("TACKON connect '" + std::string{f.at[2]} + "'", where);
    if (f.at[2].size() != 1)
      error::invalid("connect '" + std::string{f.at[2]} + "', not one letter", where);
    letters(f.at[2], "connect", where);
    fix.connect = f.at[2][0];
  }
  return fix;
}

latin::Part part(const Line& f, std::size_t i, std::string_view where) {
  return ::part(scheme::kParts, "ADDONS", f.at[i], where);
}

Addon grammar(const Fix& fix, std::string_view line, std::string_view where) {
  const Line f = split<scheme::kGrammarMaxFields>(line, scheme::kWhitespace, where);
  switch (fix.kind) {
  case Kind::prefix: {
    if (f.count != scheme::kPrefixFields)
      error::invalid("PREFIX grammar of " + std::to_string(f.count) + " fields (" +
                         std::to_string(scheme::kPrefixFields) + " needed)",
                     where);
    return addon::Prefix{fix.text, fix.connect, part(f, 0, where), part(f, 1, where)};
  }
  case Kind::tackon: {
    if (f.count < scheme::kTackonLeadFields)
      error::invalid("empty TACKON grammar", where);
    const latin::Part to = part(f, 0, where);
    const auto read = byPart<addon::Target>(to, f, scheme::kTackonLeadFields, where);
    done(f, scheme::kTackonLeadFields + read.fields, where);
    return addon::Tackon{fix.text, to, read.value};
  }
  case Kind::suffix: {
    if (f.count < scheme::kSuffixLeadFields + scheme::kSuffixTailFields)
      error::invalid("SUFFIX grammar without from, key, target and key", where);
    const latin::Part from = part(f, 0, where);
    const latin::StemKey fromKey = number<latin::StemKey>(f, 1, where);
    const latin::Part to = part(f, 2, where);
    const auto read = byPart<addon::Target>(to, f, scheme::kSuffixLeadFields, where);
    const std::size_t at = scheme::kSuffixLeadFields + read.fields;
    if (f.count != at + scheme::kSuffixTailFields)
      error::invalid("SUFFIX without one key after the target grammar", where);
    return addon::Suffix{fix.text, fix.connect, from, fromKey, to, read.value, number<latin::StemKey>(f, at, where)};
  }
  }
  std::unreachable();
}

AddonKind kindOf(const Addon& addon, std::string_view meaning) {
  if (const auto* prefix = std::get_if<addon::Prefix>(&addon))
    return prefix->to == Part::PACK ? AddonKind::Tickon : AddonKind::Prefix;
  if (std::holds_alternative<addon::Suffix>(addon))
    return AddonKind::Suffix;
  const auto* packon = std::get_if<Packon>(&std::get<addon::Tackon>(addon).grammar);
  return packon != nullptr && latin::isPackon(packon->declension, meaning) ? AddonKind::Packon : AddonKind::Tackon;
}

AddonsEntry parse(const source::AddonsLines& lines, std::string_view where) {
  const Addon addon = grammar(fix(lines[std::to_underlying(scheme::Line::fix)], where),
                              lines[std::to_underlying(scheme::Line::partEntry)], where);
  const std::string_view meaning =
      printable(trimmed(lines[std::to_underlying(scheme::Line::meaning)], scheme::kTrailingPadding), "meaning", where);
  return {.addon = addon, .meaning = meaning, .kind = kindOf(addon, meaning)};
}

} // namespace affix

} // namespace

void word::init() {
  for (std::size_t i = 0; i < dictlineScheme::kEntriesPerFile; ++i) {
    const source::DictlineIndex entry{i};
    dictline[i] = dict::parse(source::dictline(entry), where(dictlineScheme::kPath, entry));
  }
  for (std::size_t i = 0; i < inflectsScheme::kEntriesPerFile; ++i) {
    const source::InflectsIndex entry{i};
    inflects[i] = inflect::parse(source::inflects(entry), where(inflectsScheme::kPath, entry));
  }
  for (std::size_t i = 0; i < uniques.size(); ++i) {
    const source::UniquesIndex entry{i};
    uniques[i] = unique::parse(source::uniques(entry), where(uniquesScheme::kPath, entry));
  }
  for (std::size_t i = 0; i < addons.size(); ++i) {
    const source::AddonsIndex entry{i};
    addons[i] = affix::parse(source::addons(entry), where(addonsScheme::kPath, entry));
  }

  dictline[kEsse.value] = {
      .stem1 = "s",
      .stem2 = "",
      .stem3 = "fu",
      .stem4 = "fut",
      .grammar = latin::Verb{latin::Conjugation{5}, latin::Variant{1}, latin::VerbKind::TO_BE},
      .labels = {.age = latin::Age::X,
                 .area = latin::Area::X,
                 .geography = latin::Geography::X,
                 .frequency = latin::Frequency::A,
                 .source = latin::Source::X},
      .senses = "be; exist; (also used to form verb perfect passive tenses) with NOM PERF PPL",
      .packon = {},
  };
  // NOTE: Fix_Adverb's two readings need an inflection identity; these rows are it, and the join never crosses them.
  inflects[kAdverbPositive.value] = {.grammar = latin::inflected::Adverb{latin::Comparison::POS},
                                     .stemKey = latin::StemKey{0},
                                     .characterCount = latin::CharacterCount{1},
                                     .ending = "e",
                                     .age = latin::Age::X,
                                     .frequency = latin::Frequency::B};
  inflects[kAdverbSuperlative.value] = {.grammar = latin::inflected::Adverb{latin::Comparison::SUPER},
                                        .stemKey = latin::StemKey{0},
                                        .characterCount = latin::CharacterCount{2},
                                        .ending = "me",
                                        .age = latin::Age::X,
                                        .frequency = latin::Frequency::B};
}

const word::DictlineEntry& word::entry(DictlineRow row) { return dictline[row.value]; }
const word::InflectsEntry& word::entry(InflectsRow row) { return inflects[row.value]; }
const word::UniquesEntry& word::entry(UniquesRow row) { return uniques[row.value]; }
const word::AddonsEntry& word::entry(source::AddonsIndex row) { return addons[row.value]; }
