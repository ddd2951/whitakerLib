#include "atomic_publication.hpp"

#include <cerrno>
#include <cstdio>
#include <string>
#include <system_error>
#include <utility>

#include <fcntl.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {

// NOTE: Not error::fatal: returning lets Temporary clean up.
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
    if (!m_kept && !m_path.empty()) {
      std::error_code ignored;
      fs::remove(m_path, ignored);
    }
  }

  bool openBeside(const fs::path& destination, std::string& failure) {
    const std::string stem = "." + destination.filename().string() + ".tmp.";
    for (unsigned attempt = 1; attempt <= 128; ++attempt) {
      m_path = destination.parent_path() / (stem + std::to_string(attempt));
      errno = 0;
      m_file = std::fopen(m_path.c_str(), "wbx");
      if (m_file != nullptr)
        return true;
      if (errno != EEXIST) {
        m_path.clear();
        return fail(failure, "cannot create the temporary file");
      }
    }
    m_path.clear();
    return fail(failure, "no free temporary file name");
  }

  [[nodiscard]] std::FILE* file() const { return m_file; }
  [[nodiscard]] const fs::path& path() const { return m_path; }

  bool close(std::string& failure) {
    std::FILE* file = m_file;
    m_file = nullptr;
    if (std::fflush(file) != 0 || ::fsync(::fileno(file)) != 0) {
      std::fclose(file);
      return fail(failure, "cannot flush the temporary file to storage");
    }
    if (std::fclose(file) != 0)
      return fail(failure, "cannot close the temporary file");
    return true;
  }

  void keep() { m_kept = true; }

private:
  fs::path m_path;
  std::FILE* m_file{};
  bool m_kept{};
};

} // namespace

bool publish::atomically(const fs::path& destination, std::span<const unsigned char> bytes, Validator validator,
                         std::string& failure) {
  Temporary temporary;
  if (!temporary.openBeside(destination, failure))
    return false;
  if (std::fwrite(bytes.data(), 1, bytes.size(), temporary.file()) != bytes.size())
    return fail(failure, "cannot write the temporary file");
  if (!temporary.close(failure))
    return false;
  temporary.keep();
  validator(temporary.path());
  if (std::rename(temporary.path().c_str(), destination.c_str()) != 0)
    return fail(failure, "cannot rename the temporary file over the old image");
  if (const int handle = ::open(destination.parent_path().c_str(), O_RDONLY | O_DIRECTORY); handle >= 0) {
    const int synced = ::fsync(handle);
    ::close(handle);
    if (synced != 0)
      return fail(failure, "cannot sync the directory");
  }
  return true;
}
