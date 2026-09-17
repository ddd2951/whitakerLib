#pragma once

#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize::internal::inflects::post_tests {

void verifyEndingPool(const tokenized::Inflects& inflects,
                      const text::Pool& endings);

// Every declared character count fits its ending.
void verifyCharacterCounts(const tokenized::Inflects& inflects);

// Prints what the columns hold, in the shape of inflects_ledger.md's tables.
void reportInflectsLedger(const tokenized::Inflects& inflects,
                          const text::StringPools& pools);

} // namespace wl_stable::tokenize::internal::inflects::post_tests
