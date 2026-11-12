#include "execution_steps/steps.hpp"

#include "src/search/relationship_image.hpp"
#include "storage/image_spellings.hpp"

namespace rel = whitaker::relationship;

void execution_steps::image_file::enumerateImageSpellings() { storage::imageSpellings = rel::spellings(); }
