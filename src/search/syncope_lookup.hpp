#pragma once

#include "facts.hpp"
#include "search/relationship_image.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace whitaker {

// INFO: WORDS' syncope pass: the full form of a contracted perfect, `amasti` to `amavisti`, and the rule (a
//  latin::Syncope), or an empty form.
struct Syncope {
  std::array<char, facts::kMaxWordCharacters> letters{};
  std::uint8_t length{};
  std::uint8_t rule{};
  [[nodiscard]] std::string_view restored() const { return {letters.data(), length}; }
};
static_assert(std::is_trivially_copyable_v<Syncope>);
[[nodiscard]] Syncope syncopeLookup(std::string_view word);

} // namespace whitaker
