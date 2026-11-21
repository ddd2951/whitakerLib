#pragma once

#include <vector>

namespace emitter {

// INFO: The encoded image's bytes.
struct ImageFile {
  std::vector<unsigned char> bytes{};
};

} // namespace emitter
