#pragma once

#include <cstddef>
#include <string_view>

#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize::internal::dictline {

void line(std::string_view text, std::size_t index, std::string_view where,
          text::StringPools& pools, tokenized::Dictline& into);

} // namespace wl_stable::tokenize::internal::dictline
