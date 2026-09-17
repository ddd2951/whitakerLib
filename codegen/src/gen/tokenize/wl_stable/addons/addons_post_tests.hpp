#pragma once

#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize::internal::addons::post_tests {

void verifyPools(const tokenized::Addons& addons,
                 const text::StringPools& pools);

// Prints what the columns hold, in the shape of addons_ledger.md's tables.
void reportAddonsLedger(const tokenized::Addons& addons,
                        const text::StringPools& pools);

} // namespace wl_stable::tokenize::internal::addons::post_tests
