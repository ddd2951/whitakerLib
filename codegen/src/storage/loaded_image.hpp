#pragma once

#include <vector>

namespace storage {

// NOTE: The loaded image points into these bytes, so they live until gen exits.
inline std::vector<char> loadedImage{};

} // namespace storage
