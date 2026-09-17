#pragma once

#include <span>
#include <string_view>

#include "source/wl_stable/dictline_scheme.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize::internal::dictline {

tokenized::Stem extractStem(
    std::span<const char, source::wl_stable::dictline::scheme::kStemWidth>
        slice,
    std::string_view where);

} // namespace wl_stable::tokenize::internal::dictline
