#include "line.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <string_view>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/pool.hpp"
#include "grammar.hpp"
#include "labels.hpp"
#include "part.hpp"
#include "senses.hpp"
#include "stem.hpp"

namespace wl_stable::tokenize::internal::dictline {

namespace {

namespace scheme = source::wl_stable::dictline::scheme;

template <std::size_t at, std::size_t width>
std::span<const char, width> slice(std::string_view text) {
  return std::span<const char, width>{text.data() + at, width};
}

template <std::size_t which>
std::span<const char, scheme::kStemWidth> stemSlice(std::string_view text) {
  return slice<scheme::kStemAt + which * scheme::kStemWidth,
               scheme::kStemWidth>(text);
}

// Absent and empty stems are views already and are stored as they are.
tokenized::Stem placeStem(tokenized::Stem stem, text::Pool& pool) {
  if (tokenized::stemState(stem) != tokenized::StemState::text)
    return stem;
  return place(stem, pool);
}

} // namespace

void line(std::string_view text, std::size_t index, std::string_view where,
          text::StringPools& pools, tokenized::Dictline& into) {
  if (text.size() < scheme::kSensesAt + scheme::kSensesMinLength)
    error::fatal(std::string{where} + ": record is " +
                 std::to_string(text.size()) +
                 " bytes, the format needs at least " +
                 std::to_string(scheme::kSensesAt + scheme::kSensesMinLength));

  into.stem1[index] =
      placeStem(extractStem(stemSlice<0>(text), where), pools.dictlineStems);
  into.stem2[index] =
      placeStem(extractStem(stemSlice<1>(text), where), pools.dictlineStems);
  into.stem3[index] =
      placeStem(extractStem(stemSlice<2>(text), where), pools.dictlineStems);
  into.stem4[index] =
      placeStem(extractStem(stemSlice<3>(text), where), pools.dictlineStems);

  const PosTokenTypes part =
      extractPart(slice<scheme::kPartAt, scheme::kPartWidth>(text), where);
  into.grammar[index] = extractGrammar(
      part, slice<scheme::kGrammarAt, scheme::kGrammarWidth>(text), where);

  const Labels labels = extractLabels(
      slice<scheme::kLabelsAt, scheme::kLabelsWidth>(text), where);
  into.age[index] = labels.age;
  into.area[index] = labels.area;
  into.geography[index] = labels.geography;
  into.frequency[index] = labels.frequency;
  into.source[index] = labels.source;

  into.senses[index] =
      place(extractSenses(text.substr(scheme::kSensesAt), where),
            pools.dictlineSenses);
}

} // namespace wl_stable::tokenize::internal::dictline
