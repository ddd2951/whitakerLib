#pragma once

#include "types/formatted_file.hpp"
#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize {

void tokenizeInflects(const formatted::InflectsSource& source,
                      text::StringPools& pools, tokenized::Inflects& into);

} // namespace wl_stable::tokenize
