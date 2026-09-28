#pragma once

#include "image_data.hpp"

namespace emitter {

class ImageFile;

[[nodiscard]] ImageFile encodeImage(const ImageData& imageData,
                                    const DirectOutputMachine& machine);

} // namespace emitter
