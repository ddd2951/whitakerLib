#include "execution_steps/steps.hpp"

#include "storage/image.hpp"

void execution_steps::finish_image_data::copyLookupStems() {
  using storage::image;
  image.stems = image.twoEnded.stems;
}
