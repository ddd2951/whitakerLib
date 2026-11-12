#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "storage/image_destination.hpp"

#include <filesystem>
#include <utility>

namespace fs = std::filesystem;

void execution_steps::image_destination::prepareImageDestination() {
  const fs::path destination{"data/whitaker.dat"};
  if (!fs::is_directory(destination.parent_path()))
    error::fatal("image: no data/ directory here; run gen from the repository root");

  storage::destination = std::move(destination);
}
