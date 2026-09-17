#pragma once

#include "types/formatted_file.hpp"
#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize {

void tokenizeDictline(const formatted::DictlineSource& source,
                      text::StringPools& pools, tokenized::Dictline& into);

} // namespace wl_stable::tokenize
