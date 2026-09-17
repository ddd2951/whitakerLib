#include "grammar.hpp"

#include <string>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/field.hpp"
#include "source/wl_stable/dictline_scheme.hpp"
#include "types/types.hpp"
#include "util/reflect_util.hpp"

namespace wl_stable::tokenize::internal::addons {

namespace scheme = source::wl_stable::addons::scheme;
namespace dictline = source::wl_stable::dictline::scheme;

namespace {

using namespace tokenized;

PosTokenTypes part(const Fields& f, std::size_t i, std::string_view where) {
  for (const std::string_view p : scheme::kParts)
    if (p == f.at[i])
      return *util::trySvToEnum<PosTokenTypes>(f.at[i]);
  refuse(f, i, where);
}

void expect(const Fields& f, std::size_t at, std::size_t arity,
            PosTokenTypes target, std::string_view where) {
  if (f.count < at + arity)
    error::fatal(std::string{where} + ": target " +
                 std::string{util::enumToSv(target)} + " states " +
                 std::to_string(arity) + " grammar fields, line has " +
                 std::to_string(f.count - at));
}

struct Read {
  addon::Target value;
  std::size_t fields;
};

Read target(PosTokenTypes part, const Fields& f, std::size_t at,
            std::string_view where) {
  switch (part) {
  case PosTokenTypes::X:
    return {addon::Any{}, scheme::kAnyFields};
  case PosTokenTypes::N:
    expect(f, at, dictline::kNounFields, part, where);
    return {Noun{number<domain::Declension>(f, at, where),
                 number<domain::DeclensionVariant>(f, at + 1, where),
                 name<TypeGender>(f, at + 2, where),
                 name<TypeNounKind>(f, at + 3, where)},
            dictline::kNounFields};
  case PosTokenTypes::PRON:
    expect(f, at, dictline::kPronounFields, part, where);
    return {Pronoun{number<domain::Declension>(f, at, where),
                    number<domain::DeclensionVariant>(f, at + 1, where),
                    name<TypePronounKind>(f, at + 2, where)},
            dictline::kPronounFields};
  case PosTokenTypes::ADJ:
    expect(f, at, dictline::kAdjectiveFields, part, where);
    return {Adjective{number<domain::Declension>(f, at, where),
                      number<domain::DeclensionVariant>(f, at + 1, where),
                      name<TypeComparison>(f, at + 2, where)},
            dictline::kAdjectiveFields};
  case PosTokenTypes::NUM:
    expect(f, at, dictline::kNumeralFields, part, where);
    return {Numeral{number<domain::Declension>(f, at, where),
                    number<domain::DeclensionVariant>(f, at + 1, where),
                    name<TypeNumeralSort>(f, at + 2, where),
                    number<domain::NumeralValue>(f, at + 3, where)},
            dictline::kNumeralFields};
  case PosTokenTypes::V:
    expect(f, at, dictline::kVerbFields, part, where);
    return {Verb{number<domain::Conjugation>(f, at, where),
                 number<domain::ConjugationVariant>(f, at + 1, where),
                 name<TypeVerbKind>(f, at + 2, where)},
            dictline::kVerbFields};
  case PosTokenTypes::ADV:
    expect(f, at, dictline::kAdverbFields, part, where);
    return {Adverb{name<TypeComparison>(f, at, where)},
            dictline::kAdverbFields};
  case PosTokenTypes::PACK:
    expect(f, at, dictline::kPackonFields, part, where);
    return {Packon{number<domain::Declension>(f, at, where),
                   number<domain::DeclensionVariant>(f, at + 1, where),
                   name<TypePackonKind>(f, at + 2, where)},
            dictline::kPackonFields};
  case PosTokenTypes::PREP:
  case PosTokenTypes::CONJ:
  case PosTokenTypes::INTERJ:
  case PosTokenTypes::SUPINE:
  case PosTokenTypes::VPAR:
  case PosTokenTypes::NONE:
    break;
  }
  error::fatal(std::string{where} + ": no addon targets part " +
               std::string{util::enumToSv(part)});
}

void done(const Fields& f, std::size_t at, std::string_view where) {
  if (f.count != at)
    error::fatal(std::string{where} + ": " + std::to_string(f.count - at) +
                 " fields left after the grammar");
}

} // namespace

tokenized::Addon extractGrammar(const Fix& fix, std::string_view line,
                                std::string_view where) {
  const Fields f =
      split<scheme::kGrammarMaxFields>(line, scheme::kWhitespace, where);
  switch (fix.kind) {
  case Kind::prefix: {
    if (f.count != scheme::kPrefixFields)
      error::fatal(std::string{where} + ": PREFIX grammar states " +
                   std::to_string(scheme::kPrefixFields) + " fields, line has " +
                   std::to_string(f.count));
    return addon::Prefix{fix.text, fix.connect, part(f, 0, where),
                         part(f, 1, where)};
  }
  case Kind::tackon: {
    if (f.count < scheme::kTackonLeadFields)
      error::fatal(std::string{where} + ": TACKON grammar is empty");
    const PosTokenTypes to = part(f, 0, where);
    const Read read = target(to, f, scheme::kTackonLeadFields, where);
    done(f, scheme::kTackonLeadFields + read.fields, where);
    return addon::Tackon{fix.text, to, read.value};
  }
  case Kind::suffix: {
    if (f.count < scheme::kSuffixLeadFields + scheme::kSuffixTailFields)
      error::fatal(std::string{where} +
                   ": SUFFIX grammar needs from, key, target, key");
    const PosTokenTypes from = part(f, 0, where);
    const domain::StemIndex fromKey = number<domain::StemIndex>(f, 1, where);
    const PosTokenTypes to = part(f, 2, where);
    const Read read = target(to, f, scheme::kSuffixLeadFields, where);
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
                         number<domain::StemIndex>(f, at, where)};
  }
  }
  error::fatal(std::string{where} + ": unknown fix kind");
}

} // namespace wl_stable::tokenize::internal::addons
