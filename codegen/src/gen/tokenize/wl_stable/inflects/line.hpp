#pragma once

#include <cstddef>
#include <string_view>

#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize::internal::inflects {

void line(std::string_view text, std::size_t index, std::string_view where,
          text::StringPools& pools, tokenized::Inflects& into);

} // namespace wl_stable::tokenize::internal::inflects
