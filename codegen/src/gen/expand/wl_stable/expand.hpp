#pragma once

#include "gen/expand/wl_stable/facts/enrolled.hpp"
#include "gen/expand/wl_stable/spine/spine.hpp"
#include "gen/roster_tag.hpp"
#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable {
namespace [[= gen::roster::RosterTag::WlStableExpand]] expand {

using Board = spine::Board<^^enrolled>;

[[= gen::roster::HookTag::Run]] void run(const tokenized::Sources& sources,
                                         const text::StringPools& pools,
                                         Board& into);
[[= gen::roster::HookTag::Post]] void post(const Board& board);
} // namespace expand
} // namespace wl_stable
