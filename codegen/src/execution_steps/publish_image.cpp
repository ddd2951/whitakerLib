#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "src/search/image.hpp"
#include "storage/encoded_image.hpp"
#include "storage/image_destination.hpp"
#include "storage/temporary_image.hpp"

#include <cstdio>
#include <print>

#include <fcntl.h>
#include <unistd.h>

namespace rel = whitaker::relationship;

void execution_steps::image_file::publishImage() {
  const auto& destination = storage::destination;
  const auto& temporary = storage::temporaryImage;
  if (std::rename(temporary.c_str(), destination.c_str()) != 0)
    error::fatal("image: cannot rename the temporary file over the old image");
  if (const int handle = ::open(destination.parent_path().c_str(), O_RDONLY | O_DIRECTORY); handle >= 0) {
    const int synced = ::fsync(handle);
    ::close(handle);
    if (synced != 0)
      error::fatal("image: cannot sync the directory");
  }
  std::println("  published {}: {} bytes (format version {})", destination.filename().string(),
               storage::encodedImage.bytes.size(), rel::kVersion);
}
