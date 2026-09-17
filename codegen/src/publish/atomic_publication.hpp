#pragma once

#include <cstdio>
#include <filesystem>
#include <functional>
#include <string>

// NOTE: We create a temporary image, only after it's validated does it
// overwrite an earlier image
namespace publish {

using Writer = std::function<bool(std::FILE*)>;

using Validator =
    std::function<bool(const std::filesystem::path&, std::string&)>;

[[nodiscard]] bool atomically(const std::filesystem::path& destination,
                              const Writer& writer, const Validator& validator,
                              std::string& failure);

} // namespace publish
