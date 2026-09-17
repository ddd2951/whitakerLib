#include "grammar.hpp"

#include <string>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/field.hpp"
#include "util/reflect_util.hpp"

namespace wl_stable::tokenize::internal::inflects {

namespace scheme = source::wl_stable::inflects::scheme;

namespace {

using namespace tokenized::inflected;

void expect(const Fields& f, std::size_t at, std::size_t arity,
            PosTokenTypes part, std::string_view where) {
  if (f.count < at + arity)
    error::fatal(std::string{where} + ": " + std::string{util::enumToSv(part)} +
                 " states " + std::to_string(arity) +
                 " grammar fields, record has " + std::to_string(f.count - at));
}

Grammar noun(const Fields& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kNounFields, PosTokenTypes::N, where);
  return {Noun{number<domain::Declension>(f, at, where),
               number<domain::DeclensionVariant>(f, at + 1, where),
               name<TypeCase>(f, at + 2, where),
               name<TypeNumber>(f, at + 3, where),
               name<TypeGender>(f, at + 4, where)},
          scheme::kNounFields};
}

Grammar pronoun(const Fields& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kPronounFields, PosTokenTypes::PRON, where);
  return {Pronoun{number<domain::Declension>(f, at, where),
                  number<domain::DeclensionVariant>(f, at + 1, where),
                  name<TypeCase>(f, at + 2, where),
                  name<TypeNumber>(f, at + 3, where),
                  name<TypeGender>(f, at + 4, where)},
          scheme::kPronounFields};
}

Grammar adjective(const Fields& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kAdjectiveFields, PosTokenTypes::ADJ, where);
  return {Adjective{number<domain::Declension>(f, at, where),
                    number<domain::DeclensionVariant>(f, at + 1, where),
                    name<TypeCase>(f, at + 2, where),
                    name<TypeNumber>(f, at + 3, where),
                    name<TypeGender>(f, at + 4, where),
                    name<TypeComparison>(f, at + 5, where)},
          scheme::kAdjectiveFields};
}

Grammar numeral(const Fields& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kNumeralFields, PosTokenTypes::NUM, where);
  return {Numeral{number<domain::Declension>(f, at, where),
                  number<domain::DeclensionVariant>(f, at + 1, where),
                  name<TypeCase>(f, at + 2, where),
                  name<TypeNumber>(f, at + 3, where),
                  name<TypeGender>(f, at + 4, where),
                  name<TypeNumeralSort>(f, at + 5, where)},
          scheme::kNumeralFields};
}

Grammar verb(const Fields& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kVerbFields, PosTokenTypes::V, where);
  return {Verb{number<domain::Conjugation>(f, at, where),
               number<domain::ConjugationVariant>(f, at + 1, where),
               name<TypeTense>(f, at + 2, where),
               name<TypeVoice>(f, at + 3, where),
               name<TypeMood>(f, at + 4, where),
               number<domain::Person>(f, at + 5, where),
               name<TypeNumber>(f, at + 6, where)},
          scheme::kVerbFields};
}

Grammar participle(const Fields& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kParticipleFields, PosTokenTypes::VPAR, where);
  return {
      Participle{
          number<domain::Conjugation>(f, at, where),
          number<domain::ConjugationVariant>(f, at + 1, where),
          name<TypeCase>(f, at + 2, where), name<TypeNumber>(f, at + 3, where),
          name<TypeGender>(f, at + 4, where), name<TypeTense>(f, at + 5, where),
          name<TypeVoice>(f, at + 6, where), name<TypeMood>(f, at + 7, where)},
      scheme::kParticipleFields};
}

Grammar supine(const Fields& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kSupineFields, PosTokenTypes::SUPINE, where);
  return {Supine{number<domain::Conjugation>(f, at, where),
                 number<domain::ConjugationVariant>(f, at + 1, where),
                 name<TypeCase>(f, at + 2, where),
                 name<TypeNumber>(f, at + 3, where),
                 name<TypeGender>(f, at + 4, where)},
          scheme::kSupineFields};
}

Grammar adverb(const Fields& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kAdverbFields, PosTokenTypes::ADV, where);
  return {Adverb{name<TypeComparison>(f, at, where)}, scheme::kAdverbFields};
}

Grammar preposition(const Fields& f, std::size_t at, std::string_view where) {
  expect(f, at, scheme::kPrepositionFields, PosTokenTypes::PREP, where);
  return {Preposition{name<TypeCase>(f, at, where)},
          scheme::kPrepositionFields};
}

} // namespace

Grammar extractGrammar(PosTokenTypes part, const Fields& f, std::size_t at,
                       std::string_view where) {
  switch (part) {
  case PosTokenTypes::N:
    return noun(f, at, where);
  case PosTokenTypes::PRON:
    return pronoun(f, at, where);
  case PosTokenTypes::ADJ:
    return adjective(f, at, where);
  case PosTokenTypes::NUM:
    return numeral(f, at, where);
  case PosTokenTypes::V:
    return verb(f, at, where);
  case PosTokenTypes::VPAR:
    return participle(f, at, where);
  case PosTokenTypes::SUPINE:
    return supine(f, at, where);
  case PosTokenTypes::ADV:
    return adverb(f, at, where);
  case PosTokenTypes::PREP:
    return preposition(f, at, where);
  case PosTokenTypes::CONJ:
    return {Conjunction{}, scheme::kConjunctionFields};
  case PosTokenTypes::INTERJ:
    return {Interjection{}, scheme::kInterjectionFields};
  case PosTokenTypes::PACK:
  case PosTokenTypes::NONE:
  case PosTokenTypes::X:
    break;
  }
  error::fatal(std::string{where} + ": part is not an INFLECTS part");
}

} // namespace wl_stable::tokenize::internal::inflects
