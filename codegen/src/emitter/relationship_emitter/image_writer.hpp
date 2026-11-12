#pragma once

#include "image_data.hpp"

namespace emitter {

struct ImageFile;

[[nodiscard]] ImageFile encodeImage(const ImageData& imageData);

} // namespace emitter
