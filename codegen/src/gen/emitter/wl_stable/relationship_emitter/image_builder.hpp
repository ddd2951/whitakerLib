#pragma once

#include "image_data.hpp"

namespace tokenized {
struct Sources;
}

namespace wl_stable::expand {
struct Synthetics;
struct Forms;
struct Readings;
struct Swept;
} // namespace wl_stable::expand

namespace wl_stable::emitter::detail::relationship_image {

[[nodiscard]] ImageData buildImageData(const tokenized::Sources& sources,
                                       const expand::Synthetics& synthetics,
                                       const expand::Forms& forms,
                                       const expand::Readings& readings,
                                       const expand::Swept& swept);

} // namespace wl_stable::emitter::detail::relationship_image
