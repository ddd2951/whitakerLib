#pragma once
#include "search/two_ended_schema.hpp"
#include <string>
#include <vector>
namespace emitter {
struct TwoEnded {
  std::vector<whitaker::two_ended::Node> back{}, affixBack{}, prefixFront{}, suffixBack{};
  std::vector<whitaker::relationship::StemRecord> stems{};
  std::vector<std::string> stemTexts{};
  std::vector<std::uint64_t> sharedFront{};
  std::vector<std::uint16_t> frontRuns{};
  std::vector<whitaker::two_ended::Rule> rules{};
  std::vector<std::string> ruleSuffixes{};
  std::vector<whitaker::two_ended::Cell> cells{};
  std::vector<whitaker::relationship::CellIndex> cellIndex{};
  std::vector<std::uint16_t> cellOf{}, rowIds{}, addonIds{};
  std::string metadata{};
};
} // namespace emitter
