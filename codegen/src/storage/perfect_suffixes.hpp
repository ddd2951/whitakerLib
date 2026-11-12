#pragma once

#include <set>
#include <string>

namespace storage {

// INFO: Syncope returns only a restored form whose last entry has a verb reading on the perfect stem (stem column 2,
//  WORDS' key 3); such a spelling ends in one of these folded suffixes.
inline std::set<std::string> perfectSuffixes{};

} // namespace storage
