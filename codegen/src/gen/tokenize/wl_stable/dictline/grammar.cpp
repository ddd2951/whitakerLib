#include "grammar.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/field.hpp"
#include "gen/tokenize/wl_stable/common/split.hpp"
#include "util/reflect_util.hpp"

namespace wl_stable::tokenize::internal::dictline {

namespace scheme = source::wl_stable::dictline::scheme;

namespace {

using namespace tokenized;

using Fields = internal::Fields<scheme::kGrammarMaxFields>;

void expect(const Fields& f, std::size_t arity, PosTokenTypes part,
            std::string_view where) {
  if (f.count != arity)
    error::fatal(std::string{where} + ": " + std::string{util::enumToSv(part)} +
                 " states " + std::to_string(arity) +
                 " grammar fields, record has " + std::to_string(f.count));
}

Noun noun(const Fields& f, std::string_view where) {
  expect(f, scheme::kNounFields, PosTokenTypes::N, where);
  return {number<domain::Declension>(f, 0, where),
          number<domain::DeclensionVariant>(f, 1, where),
          name<TypeGender>(f, 2, where), name<TypeNounKind>(f, 3, where)};
}

Pronoun pronoun(const Fields& f, std::string_view where) {
  expect(f, scheme::kPronounFields, PosTokenTypes::PRON, where);
  return {number<domain::Declension>(f, 0, where),
          number<domain::DeclensionVariant>(f, 1, where),
          name<TypePronounKind>(f, 2, where)};
}

Verb verb(const Fields& f, std::string_view where) {
  expect(f, scheme::kVerbFields, PosTokenTypes::V, where);
  return {number<domain::Conjugation>(f, 0, where),
          number<domain::ConjugationVariant>(f, 1, where),
          name<TypeVerbKind>(f, 2, where)};
}

Adjective adjective(const Fields& f, std::string_view where) {
  expect(f, scheme::kAdjectiveFields, PosTokenTypes::ADJ, where);
  return {number<domain::Declension>(f, 0, where),
          number<domain::DeclensionVariant>(f, 1, where),
          name<TypeComparison>(f, 2, where)};
}

Numeral numeral(const Fields& f, std::string_view where) {
  expect(f, scheme::kNumeralFields, PosTokenTypes::NUM, where);
  return {number<domain::Declension>(f, 0, where),
          number<domain::DeclensionVariant>(f, 1, where),
          name<TypeNumeralSort>(f, 2, where),
          number<domain::NumeralValue>(f, 3, where)};
}

Packon packon(const Fields& f, std::string_view where) {
  expect(f, scheme::kPackonFields, PosTokenTypes::PACK, where);
  return {number<domain::Declension>(f, 0, where),
          number<domain::DeclensionVariant>(f, 1, where),
          name<TypePackonKind>(f, 2, where)};
}

Adverb adverb(const Fields& f, std::string_view where) {
  expect(f, scheme::kAdverbFields, PosTokenTypes::ADV, where);
  return {name<TypeComparison>(f, 0, where)};
}

Preposition preposition(const Fields& f, std::string_view where) {
  expect(f, scheme::kPrepositionFields, PosTokenTypes::PREP, where);
  return {name<TypeCase>(f, 0, where)};
}

} // namespace

Grammar extractGrammar(PosTokenTypes part,
                       std::span<const char, scheme::kGrammarWidth> slice,
                       std::string_view where) {
  const Fields f = split<scheme::kGrammarMaxFields>(
      std::string_view{slice.data(), slice.size()},
      std::string_view{&scheme::kFieldSeparator, 1}, where);
  switch (part) {
  case PosTokenTypes::N:
    return noun(f, where);
  case PosTokenTypes::PRON:
    return pronoun(f, where);
  case PosTokenTypes::V:
    return verb(f, where);
  case PosTokenTypes::ADJ:
    return adjective(f, where);
  case PosTokenTypes::NUM:
    return numeral(f, where);
  case PosTokenTypes::PACK:
    return packon(f, where);
  case PosTokenTypes::ADV:
    return adverb(f, where);
  case PosTokenTypes::PREP:
    return preposition(f, where);
  case PosTokenTypes::CONJ:
    expect(f, scheme::kConjunctionFields, part, where);
    return Conjunction{};
  case PosTokenTypes::INTERJ:
    expect(f, scheme::kInterjectionFields, part, where);
    return Interjection{};
  case PosTokenTypes::SUPINE:
  case PosTokenTypes::VPAR:
  case PosTokenTypes::NONE:
  case PosTokenTypes::X:
    break;
  }
  error::fatal(std::string{where} + ": part is not a DICTLINE part");
}

} // namespace wl_stable::tokenize::internal::dictline
