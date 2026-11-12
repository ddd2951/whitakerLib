#pragma once

#include <utility>
#include <vector>

namespace storage {

using ClassKey = std::pair<std::vector<unsigned>, std::vector<bool>>;

inline std::vector<ClassKey> classes{};

} // namespace storage
