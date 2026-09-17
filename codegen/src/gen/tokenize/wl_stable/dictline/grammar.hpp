#pragma once

#include <span>
#include <string_view>

#include "source/wl_stable/dictline_scheme.hpp"
#include "types/tokenized_sources.hpp"
#include "types/types.hpp"

namespace wl_stable::tokenize::internal::dictline {

tokenized::Grammar extractGrammar(
    PosTokenTypes part,
    std::span<const char, source::wl_stable::dictline::scheme::kGrammarWidth>
        slice,
    std::string_view where);

} // namespace wl_stable::tokenize::internal::dictline
