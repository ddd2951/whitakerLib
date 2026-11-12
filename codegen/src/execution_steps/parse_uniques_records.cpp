#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "helpers/expect.hpp"
#include "helpers/fields.hpp"
#include "helpers/fill.hpp"
#include "helpers/labels_follow.hpp"
#include "helpers/letter.hpp"
#include "helpers/letters.hpp"
#include "helpers/name.hpp"
#include "helpers/part.hpp"
#include "helpers/printable.hpp"
#include "helpers/split.hpp"
#include "helpers/trimmed.hpp"
#include "helpers/where.hpp"
#include "source/schemes.hpp"
#include "storage/uniques.hpp"
#include "util/reflect_util.hpp"

#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

namespace {

namespace inflectsScheme = source::scheme::inflects;
namespace uniquesScheme = source::scheme::uniques;
using helpers::expect;
using helpers::Fields;
using helpers::fill;
using helpers::labelsFollow;
using helpers::letter;
using helpers::letters;
using helpers::name;
using helpers::part;
using helpers::printable;
using helpers::split;
using helpers::trimmed;
using helpers::where;
using word::UniquesEntry;

namespace unique {

namespace scheme = uniquesScheme;
// TODO: Separate UNIQUES' field limit from INFLECTS.
using Line = Fields<inflectsScheme::kMaxFields>;
using namespace latin::inflected;

struct Grammar {
  latin::Entry word{};
  latin::Inflection form{};
  std::size_t fields{};
};
static_assert(std::is_trivially_copyable_v<Grammar>);

Grammar noun(const Line& f, const std::size_t at, const std::string_view where) {
  constexpr std::size_t form = util::kMembers<Noun>.size();
  expect(f, at, form + 1, latin::Part::N, where);
  const auto read = fill<Noun>(f, at, where);
  return {latin::Noun{read.declension, read.declensionVariant, read.gender, name<latin::NounKind>(f, at + form, where)},
          read, form + 1};
}

Grammar pronoun(const Line& f, const std::size_t at, const std::string_view where) {
  constexpr std::size_t form = util::kMembers<Pronoun>.size();
  expect(f, at, form + 1, latin::Part::PRON, where);
  const auto read = fill<Pronoun>(f, at, where);
  return {latin::Pronoun{read.declension, read.declensionVariant, name<latin::PronounKind>(f, at + form, where)}, read,
          form + 1};
}

Grammar adjective(const Line& f, const std::size_t at, const std::string_view where) {
  constexpr std::size_t form = util::kMembers<Adjective>.size();
  expect(f, at, form, latin::Part::ADJ, where);
  const auto read = fill<Adjective>(f, at, where);
  return {latin::Adjective{read.declension, read.declensionVariant, read.comparison}, read, form};
}

Grammar verb(const Line& f, const std::size_t at, const std::string_view where) {
  constexpr std::size_t form = util::kMembers<Verb>.size();
  expect(f, at, form + 1, latin::Part::V, where);
  const auto read = fill<Verb>(f, at, where);
  return {latin::Verb{read.conjugation, read.conjugationVariant, name<latin::VerbKind>(f, at + form, where)}, read,
          form + 1};
}

Grammar grammar(const latin::Part part, const Line& f, const std::size_t at, const std::string_view where) {
  switch (part) {
    case latin::Part::N:
      return noun(f, at, where);
    case latin::Part::PRON:
      return pronoun(f, at, where);
    case latin::Part::ADJ:
      return adjective(f, at, where);
    case latin::Part::V:
      return verb(f, at, where);
    case latin::Part::X:
    case latin::Part::ADV:
    case latin::Part::PREP:
    case latin::Part::NUM:
    case latin::Part::CONJ:
    case latin::Part::INTERJ:
    case latin::Part::PACK:
    case latin::Part::SUPINE:
    case latin::Part::VPAR:
    case latin::Part::NONE:
      break;
  }
  std::unreachable();
}

std::string_view form(const std::string_view word, const std::string_view where) {
  const std::string_view form = trimmed(word, scheme::kTrailingPadding);
  if (form.empty())
    error::invalid("blank word line", where);
  letters(form, "word", where);
  return form;
}

UniquesEntry parse(const source::UniquesLines& lines, const std::string_view where) {
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

} // namespace

void execution_steps::uniques::parseUniquesRecords() {
  using storage::uniques;
  for (std::size_t i = 0; i < uniques.size(); ++i) {
    const source::UniquesIndex entry{i};
    uniques[i] = unique::parse(source::uniques(entry), where(uniquesScheme::kPath, entry));
  }
}
