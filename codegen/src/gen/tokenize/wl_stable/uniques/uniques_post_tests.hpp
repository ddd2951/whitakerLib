#pragma once

#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize::internal::uniques::post_tests {

void verifyPools(const tokenized::Uniques& uniques,
                 const text::StringPools& pools);

// Prints what the columns hold, in the shape of uniques_ledger.md's tables.
void reportUniquesLedger(const tokenized::Uniques& uniques,
                         const text::StringPools& pools);

} // namespace wl_stable::tokenize::internal::uniques::post_tests
