#pragma once

#include <map>
#include <utility>
#include <vector>

namespace storage {

// INFO: A column's rule list has already checked licensing and columnForKey. Allowed_Stem must wait until
//  after rarity evidence, just as in List_Sweep.
inline std::map<std::pair<unsigned, unsigned>, std::vector<unsigned>> byStem{};

} // namespace storage
