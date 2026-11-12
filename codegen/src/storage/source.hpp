#pragma once

#include "emitter/relationship_emitter/image_data.hpp"

#include <string>
#include <vector>

namespace storage {

using emitter::Analysis;

struct SourceRow {
  std::string word{};
  Analysis analysis{};
};

inline std::vector<SourceRow> source{};

} // namespace storage
