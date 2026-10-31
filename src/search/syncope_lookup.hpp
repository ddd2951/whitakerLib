#pragma once

#include "facts.hpp"
#include "search/relationship_image.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace whitaker {

// WORDS' syncope pass: the full form of a contracted perfect, `amasti` to `amavisti`, and the rule (a
//  WhitakerRoute), or an empty form.
struct Syncope {
  std::array<char, facts::kMaxWordCharacters> letters{};
  std::uint8_t length{};
  std::uint8_t rule{};
  [[nodiscard]] std::string_view restored() const noexcept { return {letters.data(), length}; }
};
[[nodiscard]] Syncope syncopeLookup(std::string_view word) noexcept;

} // namespace whitaker
