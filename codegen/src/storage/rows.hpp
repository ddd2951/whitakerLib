#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace storage {

using RuleKey =
    std::tuple<unsigned, std::string, std::string, std::uint64_t, std::uint64_t, unsigned, unsigned, unsigned, int>;

inline std::vector<RuleKey> rows{};

} // namespace storage
