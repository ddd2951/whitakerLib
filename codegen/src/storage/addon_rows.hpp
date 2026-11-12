#pragma once

#include "types/grammar.hpp"
#include "util/reflect_util.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace storage {

using AddonRows = std::array<std::vector<std::size_t>, util::kEnumerators<latin::AddonKind>.size()>;

inline AddonRows addonRows{};

} // namespace storage
