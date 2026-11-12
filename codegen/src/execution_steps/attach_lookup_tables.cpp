#include "execution_steps/steps.hpp"

#include "storage/image.hpp"
#include "storage/result.hpp"

void execution_steps::finish_image_data::attachLookupTables() { storage::image.twoEnded = storage::result; }
