#pragma once

#include <filesystem>
#include <vector>

// The one way the generator reads a file. Whole file, exactly the bytes on
// disk, nothing translated. A file that cannot be read ends the run.
namespace util {

[[nodiscard]] std::vector<char> fileLoad(const std::filesystem::path& path);

} // namespace util
