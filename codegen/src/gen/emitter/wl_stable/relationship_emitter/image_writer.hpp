#pragma once

#include "image_data.hpp"

namespace wl_stable::emitter {
class ImageFile;
}

namespace wl_stable::emitter::detail::relationship_image {

[[nodiscard]] ImageFile
encodeImage(const ImageData& imageData,
            const relationship_index::DirectOutputMachine& machine);

} // namespace wl_stable::emitter::detail::relationship_image
