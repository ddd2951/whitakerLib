#pragma once

#include "search/relationship_image.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace whitaker {

// NOTE: `orth` is a slice of the query, not an image string: a suffix reading keeps the fix on, a prefix reading takes
//  it off.
struct FallbackMatch {
  std::uint32_t orthOffset{};
  std::uint32_t orthLength{};
  latin::Analysis grammar{};
  std::uint16_t dictionary{};
  std::array<std::uint16_t, 2> addonId{};
  std::array<latin::AddonKind, 2> addonKind{};
  std::uint8_t addonCount{};
};

// INFO: WORDS calls this only for an unanswered word. Prune_Stems tries
//  prefixes first, then suffixes only if prefixes find nothing.
// NOTE: The span is thread-local and lives until the next call.
[[nodiscard]] std::span<const FallbackMatch> addonFallback(std::string_view word) noexcept;

// INFO: Not a fallback: WORDS runs it inside Word. Same buffer as above.
[[nodiscard]] std::span<const FallbackMatch> packonReadings(std::string_view word) noexcept;

} // namespace whitaker
