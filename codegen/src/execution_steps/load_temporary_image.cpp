#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "src/search/relationship_image.hpp"
#include "storage/loaded_image.hpp"
#include "storage/temporary_image.hpp"
#include "util/file_load.hpp"

#include <span>
#include <string>

namespace rel = whitaker::relationship;

void execution_steps::image_file::loadTemporaryImage() {
  storage::loadedImage = util::fileLoad(storage::temporaryImage);
  std::string failure{};
  if (!rel::loadImage(std::as_bytes(std::span{storage::loadedImage}), failure))
    error::fatal(failure);
}
