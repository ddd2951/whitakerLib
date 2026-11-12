#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace storage {

using DescriptionIds = std::map<std::string, std::uint32_t>;

inline DescriptionIds descriptionIds{};

} // namespace storage
