#pragma once

#include <cstddef>
#include <string_view>

#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize::internal::uniques {

void line(std::string_view word, std::string_view attributes,
          std::string_view meaning, std::size_t index, std::string_view where,
          text::StringPools& pools, tokenized::Uniques& into);

} // namespace wl_stable::tokenize::internal::uniques
