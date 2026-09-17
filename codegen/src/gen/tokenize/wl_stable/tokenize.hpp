#pragma once

#include "gen/roster_tag.hpp"
#include "types/formatted_file.hpp"
#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable {
namespace[[= gen::roster::RosterTag::WlStableTokenize]] tokenize {
[[= gen::roster::HookTag::Pre]] void pre();
[[= gen::roster::HookTag::Run]] void run(const formatted::Sources& formatted,
                                         text::StringPools& pools,
                                         tokenized::Sources& into);
[[= gen::roster::HookTag::Post]] void post(const tokenized::Sources& tokenized,
                                           const text::StringPools& pools);
} // namespace tokenize
} // namespace wl_stable
