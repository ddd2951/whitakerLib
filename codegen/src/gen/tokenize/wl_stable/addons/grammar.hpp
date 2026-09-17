#pragma once

#include <cstddef>
#include <string_view>

#include "fix.hpp"
#include "gen/tokenize/wl_stable/common/split.hpp"
#include "source/wl_stable/addons_scheme.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize::internal::addons {

using Fields =
    internal::Fields<source::wl_stable::addons::scheme::kGrammarMaxFields>;

tokenized::Addon extractGrammar(const Fix& fix, std::string_view line,
                                std::string_view where);

} // namespace wl_stable::tokenize::internal::addons
