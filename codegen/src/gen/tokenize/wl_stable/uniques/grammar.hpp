#pragma once

#include <cstddef>
#include <string_view>

#include "gen/tokenize/wl_stable/common/split.hpp"
#include "source/wl_stable/uniques_scheme.hpp"
#include "types/tokenized_sources.hpp"
#include "types/types.hpp"

namespace wl_stable::tokenize::internal::uniques {

using Fields = internal::Fields<source::wl_stable::uniques::scheme::kMaxFields>;

struct Grammar {
  tokenized::UniqueGrammar value;
  std::size_t fields;
};

Grammar extractGrammar(PosTokenTypes part, const Fields& f, std::size_t at,
                       std::string_view where);

} // namespace wl_stable::tokenize::internal::uniques
