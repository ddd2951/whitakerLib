#pragma once

#include "facts.hpp"
#include "latin.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace whitaker {

struct AddonStep {
  std::uint16_t id{};
  latin::AddonKind kind{};
};

struct AddonReading {
  std::uint32_t orthOffsetInWord{};
  std::uint32_t orthLength{};
  latin::Analysis grammar{};
  std::uint16_t dictionary{};
  std::array<AddonStep, 2> addons{};
  std::uint8_t addonCount{};
  latin::Age inflectionAge{};
  latin::Frequency inflectionFrequency{};
  std::array<char, facts::kMaxWordCharacters> endingAsLookedUp{};
  std::uint8_t endingLength{};
};

// NOTE: The readings live until the next call on this thread.
struct AddonReadings {
  std::span<const AddonReading> readings;
  bool trimmed{};
  bool outOfMemory{};
};

[[nodiscard]] AddonReadings affixReadings(std::string_view word) noexcept;

[[nodiscard]] AddonReadings packonReadings(std::string_view word) noexcept;

} // namespace whitaker
