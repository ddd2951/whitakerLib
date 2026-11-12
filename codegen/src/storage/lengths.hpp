#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <tuple>

namespace storage {

inline std::map<std::tuple<std::string, unsigned, std::uint64_t>, unsigned> lengths{};

} // namespace storage
