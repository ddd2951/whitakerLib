#pragma once

#include "latin.hpp"
#include <string>
#include <vector>

namespace storage {

struct SourceStem {
  std::string text{};
  unsigned dictionary{}, column{}, flags{}, klass{};
  unsigned stemKey = 255;
  latin::Part part = latin::Part::NONE;
};

inline std::vector<SourceStem> stems{};

} // namespace storage
