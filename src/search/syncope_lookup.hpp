#pragma once

#include "search/relationship_image.hpp"

#include <string>
#include <string_view>

namespace whitaker {

// WORDS' syncope pass: the full form of a contracted perfect, `amasti` to `amavisti`, or empty.
[[nodiscard]] std::string syncopeLookup(std::string_view word);

} // namespace whitaker
