#pragma once

#include "search/relationship_image.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

// INFO: WORDS runs fixes only for a word the plain search answered nothing
//  for. Calling this for any other word is wrong, not wasteful.
namespace whitaker {

// NOTE: `orth` is a slice of the query, not an image string: a suffix reading
//  keeps the fix on, a prefix reading takes it off.
struct FallbackMatch {
  std::uint32_t orthOffset{};
  std::uint32_t orthLength{};
  const char* meaning{};
  const char* pos{};
  const char* inflection{};
  std::array<const char*, 2> addonSpelling{};
  std::array<const char*, 2> addonMeaning{};
  std::array<relationship::AddonKind, 2> addonKind{};
  std::uint8_t addonCount{};
};

// INFO: Prune_Stems' precedence: prefixes first, suffixes only if nothing.
// NOTE: The span is thread-local and lives until the next call.
[[nodiscard]] std::span<const FallbackMatch>
addonFallback(const relationship::Image& image, std::string_view word) noexcept;

// INFO: Not a fallback: WORDS runs it inside Word. Same buffer as above.
[[nodiscard]] std::span<const FallbackMatch>
packonReadings(const relationship::Image& image,
               std::string_view word) noexcept;

} // namespace whitaker
