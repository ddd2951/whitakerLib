#pragma once

#include "facts.hpp"
#include "relationship_index.hpp"
#include "src/search/relationship_schema.hpp"
#include "string_section.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace emitter {

struct DictionaryRecord {
  std::array<std::uint32_t, facts::kStemColumnCount> orth{};
  std::uint32_t meaning{};
  std::uint8_t age{};
  std::uint8_t frequency{};
  std::uint16_t classId{};
};

struct AddonRecord {
  std::uint32_t fix{};
  std::uint32_t meaning{};
  std::array<std::uint16_t, 4> target{};
  latin::AddonKind kind{};
  latin::Part root{};
  latin::Part targetPart{};
  std::uint8_t rootKey{};
  std::uint8_t targetKey{};
  std::uint8_t connect{};
  std::uint8_t targetGate{};
  std::uint8_t firstRaw{};
  std::uint32_t rowStart{};
  std::uint16_t rowCount{};
};

struct FallbackRowRecord {
  std::uint16_t inflect{};
  std::uint16_t description{};
};

struct ClassRecord {
  std::uint32_t rowStart{};
  std::uint16_t rowCount{};
  std::uint16_t gate{};
};

struct InflectionRecord {
  std::uint32_t ending{};
  std::uint8_t key{};
  std::uint8_t allow{};
  std::uint8_t age{};
  std::uint8_t frequency{};
};

struct EndingRecord {
  std::uint32_t text{};
  std::uint16_t first{};
  std::uint16_t count{};
};

struct FallbackStemRecord {
  std::uint32_t text{};
  std::uint16_t dictionary{};
  std::uint8_t key{};
  latin::Part part{};
};

struct ImageData {
  StringSection strings;
  std::vector<DictionaryRecord> dictionaries;
  std::vector<latin::Analysis> grammars;
  std::vector<whitaker::relationship::Entry> entries;
  std::vector<AddonRecord> addons;
  std::vector<ClassRecord> classes;
  std::vector<FallbackRowRecord> fallbackRows;
  std::vector<InflectionRecord> inflections;
  std::vector<EndingRecord> endings;
  std::vector<FallbackStemRecord> fallbackStems;
  std::vector<Word> words;
  std::vector<Analysis> analyses;
  Program program;
};

} // namespace emitter
