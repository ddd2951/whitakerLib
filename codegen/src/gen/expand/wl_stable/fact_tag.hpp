#pragma once

namespace wl_stable::expand {

// NOTE: Fact: reads the board values its const parameters name and returns the
// one value it adds.
// NOTE: Report: reads one fact and prints; run at post, and only
//  where declared.
enum class FactTag { Fact, Report };

} // namespace wl_stable::expand
