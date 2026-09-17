#pragma once

#include "types/formatted_file.hpp"
#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize {

void tokenizeAddons(const formatted::AddonsSource& source,
                    text::StringPools& pools, tokenized::Addons& into);

} // namespace wl_stable::tokenize
