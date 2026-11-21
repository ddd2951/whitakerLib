#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "storage/image_destination.hpp"
#include "storage/temporary_image.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

#include <fcntl.h>
#include <unistd.h>

void execution_steps::image_file::publishImage() {
  const auto& destination = storage::destination;
  const auto& temporary = storage::temporaryImage;
  if (std::rename(temporary.c_str(), destination.c_str()) != 0)
    error::fatal("image: cannot rename " + temporary.string() + " over " + destination.string() + ": " +
                 std::strerror(errno));
  const auto directory = destination.parent_path();
  const int handle = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY);
  if (handle < 0)
    error::fatal("image: cannot open " + directory.string() + ": " + std::strerror(errno));
  const int synced = ::fsync(handle);
  const int cause = errno;
  ::close(handle);
  if (synced != 0)
    error::fatal("image: cannot sync " + directory.string() + ": " + std::strerror(cause));
}
