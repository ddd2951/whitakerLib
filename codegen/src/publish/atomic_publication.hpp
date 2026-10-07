#pragma once

#include <filesystem>
#include <span>
#include <string>

namespace publish {

using Validator = void (*)(const std::filesystem::path&);

[[nodiscard]] bool atomically(const std::filesystem::path& destination, std::span<const unsigned char> bytes,
                              Validator validator, std::string& failure);

} // namespace publish
