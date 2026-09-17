#pragma once

#include "gen/roster_tag.hpp"
#include "types/formatted_file.hpp"

// INFO: Construction role is to
// Take the four source files as input
// Return to tokenizer the ownership of a formatted::Sources
// Nothing construction read outlives it, only the lines it hands on
// A file that does not hold the line count its scheme states ends the run
// There is no pre, the count check is inside run and the proof is in post
// post does not just look at the result, it reads the sources again and
// rebuilds them from it, this is a known and accepted cost

namespace wl_stable {
namespace[[= gen::roster::RosterTag::WlStableConstruction]] construction {

// NOTE: pre turned out empty, so until it has content we comment it out for the
// spine
//  [[= gen::roster::HookTag::Pre]] void pre();
[[= gen::roster::HookTag::Run]] formatted::Sources run();
[[= gen::roster::HookTag::Post]] void post(const formatted::Sources& sources);
} // namespace construction
} // namespace wl_stable
