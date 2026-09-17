#include "line.hpp"

#include <string>

#include "error/error.hpp"
#include "form.hpp"
#include "gen/tokenize/wl_stable/common/field.hpp"
#include "gen/tokenize/wl_stable/common/pool.hpp"
#include "gen/tokenize/wl_stable/common/split.hpp"
#include "grammar.hpp"
#include "part.hpp"
#include "senses.hpp"
#include "source/wl_stable/uniques_scheme.hpp"
#include "types/types.hpp"

namespace wl_stable::tokenize::internal::uniques {

namespace scheme = source::wl_stable::uniques::scheme;

void line(std::string_view word, std::string_view attributes,
          std::string_view meaning, std::size_t index, std::string_view where,
          text::StringPools& pools, tokenized::Uniques& into) {
  into.form[index] = place(extractForm(word, where), pools.uniquesForms);

  const Fields f =
      split<scheme::kMaxFields>(attributes, scheme::kWhitespace, where);
  if (f.count == 0)
    error::fatal(std::string{where} + ": attribute line has no fields");
  std::size_t at = 1;
  const PosTokenTypes part = extractPart(f.at[0], where);
  const Grammar grammar = extractGrammar(part, f, at, where);
  at += grammar.fields;
  if (f.count != at + scheme::kLabelCount)
    error::fatal(std::string{where} + ": " + std::to_string(f.count - at) +
                 " fields after the grammar, the format states " +
                 std::to_string(scheme::kLabelCount));
  into.grammar[index] = grammar.value;
  into.age[index] = letter<TypeAge>(f, at, where);
  into.area[index] = letter<TypeArea>(f, at + 1, where);
  into.geography[index] = letter<TypeGeography>(f, at + 2, where);
  into.frequency[index] = letter<TypeFrequency>(f, at + 3, where);
  into.source[index] = letter<TypeSource>(f, at + 4, where);

  into.senses[index] =
      place(extractSenses(meaning, where), pools.uniquesSenses);
}

} // namespace wl_stable::tokenize::internal::uniques
