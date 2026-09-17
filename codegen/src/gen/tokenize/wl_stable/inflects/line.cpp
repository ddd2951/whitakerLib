#include "line.hpp"

#include <string>

#include "ending.hpp"
#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/field.hpp"
#include "gen/tokenize/wl_stable/common/pool.hpp"
#include "gen/tokenize/wl_stable/common/split.hpp"
#include "grammar.hpp"
#include "part.hpp"
#include "source/wl_stable/inflects_scheme.hpp"
#include "types/types.hpp"

namespace wl_stable::tokenize::internal::inflects {

namespace scheme = source::wl_stable::inflects::scheme;

void line(std::string_view text, std::size_t index, std::string_view where,
          text::StringPools& pools, tokenized::Inflects& into) {
  const Fields f = split<scheme::kMaxFields>(
      text.substr(0, text.find(scheme::kCommentMarker)), scheme::kWhitespace,
      where);
  if (f.count == 0)
    error::fatal(std::string{where} + ": record has no fields");

  std::size_t at = 1;
  const PosTokenTypes part = extractPart(f.at[0], where);
  const Grammar grammar = extractGrammar(part, f, at, where);
  at += grammar.fields;
  const Ending ending = extractEnding(f, at, where);
  at += ending.fields;
  if (f.count != at + scheme::kLabelCount)
    error::fatal(std::string{where} + ": " + std::to_string(f.count - at) +
                 " fields after the ending, the format states " +
                 std::to_string(scheme::kLabelCount));

  into.grammar[index] = grammar.value;
  into.stemKey[index] = ending.stemKey;
  into.characterCount[index] = ending.characterCount;
  into.ending[index] = place(ending.text, pools.inflectsEndings);
  into.age[index] = letter<TypeAge>(f, at, where);
  into.frequency[index] = letter<TypeFrequency>(f, at + 1, where);
}

} // namespace wl_stable::tokenize::internal::inflects
