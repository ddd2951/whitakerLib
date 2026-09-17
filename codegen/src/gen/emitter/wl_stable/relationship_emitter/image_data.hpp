#pragma once

#include "facts.hpp"
#include "relationship_index.hpp"
#include "src/search/relationship_schema.hpp"
#include "string_section.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace wl_stable::emitter::detail::relationship_image {

struct DictionaryRecord {
  std::array<std::uint32_t, facts::kStemColumnCount> orth{};
  std::uint32_t meaning{};
  std::uint8_t age{};
  std::uint8_t frequency{};
  std::uint16_t classId{};
};

struct DescriptionRecord {
  std::uint32_t pos{};
  std::uint32_t inflection{};
};

struct AddonRecord {
  std::uint32_t fix{};
  std::uint32_t meaning{};
  std::array<std::uint16_t, 4> target{};
  whitaker::relationship::AddonKind kind{};
  std::uint8_t root{};
  std::uint8_t targetPart{};
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
  std::uint8_t part{};
};

struct ImageData {
  StringSection strings;
  std::vector<DictionaryRecord> dictionaries;
  std::vector<DescriptionRecord> descriptions;
  std::vector<AddonRecord> addons;
  std::vector<ClassRecord> classes;
  std::vector<FallbackRowRecord> fallbackRows;
  std::vector<InflectionRecord> inflections;
  std::vector<EndingRecord> endings;
  std::vector<FallbackStemRecord> fallbackStems;
  std::vector<relationship_index::Word> words;
  std::vector<relationship_index::Analysis> analyses;
  relationship_index::Program program;
};

} // namespace wl_stable::emitter::detail::relationship_image
