#include "atomic_publication.hpp"

#include <atomic>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <string>
#include <system_error>
#include <utility>

namespace fs = std::filesystem;

namespace {

bool fail(std::string& failure, std::string message) {
  failure = std::move(message);
  return false;
}

class Temporary {
public:
  Temporary() = default;
  Temporary(const Temporary&) = delete;
  Temporary& operator=(const Temporary&) = delete;
  ~Temporary() {
    if (m_file != nullptr)
      std::fclose(m_file);
    if (!m_path.empty()) {
      std::error_code ignored;
      fs::remove(m_path, ignored);
    }
  }

  bool openBeside(const fs::path& destination, std::string& failure) {
    static std::atomic_uint64_t s_serial{};
    const fs::path directory = destination.has_parent_path()
                                   ? destination.parent_path()
                                   : fs::path{"."};
    const std::string stem = "." + destination.filename().string() + ".tmp.";
    for (unsigned attempt = 0; attempt < 128; ++attempt) {
      m_path = directory / (stem + std::to_string(++s_serial));
      errno = 0;
      // "x": create exclusively, so two runs in the same directory cannot
      // choose the same name and write over each other.
      m_file = std::fopen(m_path.c_str(), "wbx");
      if (m_file != nullptr)
        return true;
      if (errno != EEXIST) {
        m_path.clear();
        return fail(failure,
                    "cannot create temporary image beside destination");
      }
    }
    m_path.clear();
    return fail(failure, "cannot allocate a unique temporary image name");
  }

  [[nodiscard]] std::FILE* file() const { return m_file; }
  [[nodiscard]] const fs::path& path() const { return m_path; }

  bool close(std::string& failure) {
    std::FILE* file = m_file;
    m_file = nullptr;
    if (std::fflush(file) != 0 || ::fsync(::fileno(file)) != 0) {
      std::fclose(file);
      return fail(failure, "cannot flush temporary image to storage");
    }
    if (std::fclose(file) != 0)
      return fail(failure, "cannot close temporary image");
    return true;
  }

  void committed() { m_path.clear(); }

private:
  fs::path m_path;
  std::FILE* m_file{};
};

} // namespace

bool publish::atomically(const fs::path& destination, const Writer& writer,
                         const Validator& validator, std::string& failure) {
  Temporary temporary;
  if (!temporary.openBeside(destination, failure))
    return false;
  if (!writer(temporary.file())) {
    std::string closeFailure;
    failure = "temporary image write failed";
    if (!temporary.close(closeFailure))
      failure += "; " + closeFailure;
    return false;
  }
  if (!temporary.close(failure) || !validator(temporary.path(), failure))
    return false;
  const fs::path directory =
      destination.has_parent_path() ? destination.parent_path() : fs::path{"."};
  if (std::rename(temporary.path().c_str(), destination.c_str()) != 0)
    return fail(failure, "cannot atomically replace destination image");
  temporary.committed();
  if (const int handle = ::open(directory.c_str(), O_RDONLY | O_DIRECTORY);
      handle >= 0) {
    const int synced = ::fsync(handle);
    ::close(handle);
    if (synced != 0)
      return fail(failure, "cannot record the published image in its "
                           "directory");
  }
  return true;
}
