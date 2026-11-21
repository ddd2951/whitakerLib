#pragma once

#include <compare>
#include <cstdint>
#include <string>
#include <vector>

namespace storage {

// INFO: A lookup rule's values; rules' map compares them member by member, in this order.
struct RuleKey {
  unsigned sourceRow{};
  std::string suffix{}, ending{};
  std::uint64_t grammar{}, sweep{};
  unsigned key{}, age{}, frequency{};
  int correction{};
  auto operator<=>(const RuleKey&) const = default;
};

inline std::vector<RuleKey> rows{};

} // namespace storage
