#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "storage/encoded_image.hpp"
#include "storage/image_destination.hpp"
#include "storage/temporary_image.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <span>
#include <string>
#include <system_error>
#include <utility>

#include <unistd.h>

namespace fs = std::filesystem;

namespace {

// NOTE: Not error::fatal: returning lets Temporary clean up.
bool fail(std::string& failure, std::string message) {
  failure = std::move(message);
  return false;
}

std::string reason(const fs::path& path) { return path.string() + ": " + std::strerror(errno); }

class Temporary {
public:
  Temporary() = default;
  Temporary(const Temporary&) = delete;
  Temporary& operator=(const Temporary&) = delete;
  ~Temporary() {
    if (stream != nullptr)
      std::fclose(stream);
    if (!kept && !temporaryPath.empty()) {
      std::error_code ignored{};
      fs::remove(temporaryPath, ignored);
    }
  }

  bool openBeside(const fs::path& destination, std::string& failure) {
    const std::string stem = "." + destination.filename().string() + ".tmp.";
    for (unsigned attempt = 1; attempt <= 128; ++attempt) {
      temporaryPath = destination.parent_path() / (stem + std::to_string(attempt));
      errno = 0;
      stream = std::fopen(temporaryPath.c_str(), "wbx");
      if (stream != nullptr)
        return true;
      if (errno != EEXIST) {
        fail(failure, "cannot create " + reason(temporaryPath));
        temporaryPath.clear();
        return false;
      }
    }
    temporaryPath.clear();
    return fail(failure, "no free temporary file name beside " + destination.string());
  }

  [[nodiscard]] std::FILE* file() const { return stream; }
  [[nodiscard]] const fs::path& path() const { return temporaryPath; }

  bool close(std::string& failure) {
    std::FILE* const file = stream;
    stream = nullptr;
    if (std::fflush(file) != 0 || ::fsync(::fileno(file)) != 0) {
      fail(failure, "cannot flush " + reason(temporaryPath));
      std::fclose(file);
      return false;
    }
    if (std::fclose(file) != 0)
      return fail(failure, "cannot close " + reason(temporaryPath));
    return true;
  }

  void keep() { kept = true; }

private:
  fs::path temporaryPath{};
  std::FILE* stream{};
  bool kept{};
};

// INFO: The image written beside the destination, flushed to storage and kept; an empty path when that fails.
//  publishImage renames it over the destination; a later step that stops gen leaves it there to look at.
[[nodiscard]] fs::path writeBeside(const fs::path& destination, const std::span<const unsigned char> bytes,
                                   std::string& failure) {
  Temporary temporary{};
  if (!temporary.openBeside(destination, failure))
    return {};
  if (std::fwrite(bytes.data(), 1, bytes.size(), temporary.file()) != bytes.size()) {
    fail(failure, "cannot write " + reason(temporary.path()));
    return {};
  }
  if (!temporary.close(failure))
    return {};
  temporary.keep();
  return temporary.path();
}

} // namespace

void execution_steps::image_file::writeTemporaryImage() {
  std::string failure{};
  storage::temporaryImage = writeBeside(storage::destination, storage::encodedImage.bytes, failure);
  if (storage::temporaryImage.empty())
    error::fatal("image: " + failure);
}
