#pragma once

#include "types/formatted_file.hpp"
#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize {

void tokenizeUniques(const formatted::UniquesSource& source,
                     text::StringPools& pools, tokenized::Uniques& into);

} // namespace wl_stable::tokenize
