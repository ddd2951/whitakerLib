#pragma once

#include "src/search/relationship_schema.hpp"
#include "string_section.hpp"
#include "readings/tables.hpp"

#include <cstdint>
#include <string>
#include <vector>
#include <type_traits>

namespace emitter {

struct Word {
  std::string text{};
  std::uint32_t analysisFirst{};
  std::uint16_t analysisCount{};
  bool trimmed{};
};

struct Analysis {
  std::uint16_t dictionary{};
  std::uint8_t stem{};
  std::uint16_t description{};
};
static_assert(std::is_trivially_copyable_v<Analysis>);

namespace rel = whitaker::relationship;

struct ImageData {
  TwoEnded twoEnded{};
  StringSection strings{};
  std::vector<std::uint32_t> stemPatterns{};
  std::vector<rel::DictionaryRecord> dictionaries{};
  std::vector<latin::Analysis> grammars{};
  std::vector<rel::Entry> entries{};
  std::vector<rel::AddonRecord> addons{};
  std::vector<rel::ClassRecord> classes{};
  std::vector<rel::ClassRowRecord> classRows{};
  std::vector<rel::InflectionRecord> inflections{};
  std::vector<rel::EndingRecord> endings{};
  std::vector<rel::InflectionLabelRecord> inflectionLabels{};
  std::vector<rel::TrimmedReadingRecord> trimmedReadings{};
  std::vector<rel::StemRecord> stems{};
  std::vector<Word> words{};
  std::vector<Analysis> analyses{};
};

} // namespace emitter
