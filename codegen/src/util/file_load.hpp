#pragma once

#include <filesystem>
#include <vector>

// INFO: Binary file reader for generator inputs; reads whole files without translation.
//  An unreadable file ends the run.
namespace util {

[[nodiscard]] std::vector<char> fileLoad(const std::filesystem::path& path);

} // namespace util
