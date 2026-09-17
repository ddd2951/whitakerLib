#include "util/file_load.hpp"

#include "error/error.hpp"

#include <fstream>
#include <ios>

namespace util {

std::vector<char> fileLoad(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary | std::ios::ate};
  if (!input)
    error::fatal(path.string() + ": cannot be opened for reading");

  const std::streamoff size = input.tellg();
  if (size < 0)
    error::fatal(path.string() + ": cannot determine file size");
  std::vector<char> bytes(static_cast<std::size_t>(size));
  input.seekg(0);
  input.read(bytes.data(), size);
  if (!input)
    error::fatal(path.string() + ": read failed part way through");

  return bytes;
}

} // namespace util
