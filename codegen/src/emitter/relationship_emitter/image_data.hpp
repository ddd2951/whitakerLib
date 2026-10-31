#pragma once

#include "relationship_index.hpp"
#include "src/search/relationship_schema.hpp"
#include "string_section.hpp"

#include <cstdint>
#include <vector>

namespace emitter {

namespace rel = whitaker::relationship;

struct ImageData {
  StringSection strings;
  std::vector<std::uint32_t> stemPatterns;
  std::vector<rel::DictionaryRecord> dictionaries;
  std::vector<latin::Analysis> grammars;
  std::vector<rel::Entry> entries;
  std::vector<rel::AddonRecord> addons;
  std::vector<rel::ClassRecord> classes;
  std::vector<rel::ClassRowRecord> classRows;
  std::vector<rel::InflectionRecord> inflections;
  std::vector<rel::EndingRecord> endings;
  std::vector<rel::InflectionLabelRecord> inflectionLabels;
  std::vector<rel::TrimmedReadingRecord> trimmedReadings;
  std::vector<rel::StemRecord> stems;
  std::vector<Word> words;
  std::vector<Analysis> analyses;
  Program program;
};

} // namespace emitter
