#pragma once

#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize::internal::dictline::post_tests {

// Verifies the following: no zzz and every stem view is inside the stem pool
void verifyStemPool(const tokenized::Dictline& dictline,
                    const text::Pool& stems);

void verifySensesPool(const tokenized::Dictline& dictline,
                      const text::Pool& senses);

// Prints what the columns hold, in the shape of dictline_ledger.md's tables.
void reportDictlineLedger(const tokenized::Dictline& dictline,
                          const text::StringPools& pools);

} // namespace wl_stable::tokenize::internal::dictline::post_tests
