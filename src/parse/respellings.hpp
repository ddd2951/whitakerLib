#pragma once

#include "parse/result.hpp"

#include <cstdint>
#include <string_view>

namespace whitaker::parse {

bool addPrefixVariantReadings(Work& out, Text& text, std::string_view input) noexcept;
bool addRespelledReadings(Work& out, Text& text, std::string_view input) noexcept;
void addRespelledEncliticReadings(Work& out, Text& text, std::string_view word) noexcept;
[[nodiscard]] WhitakerSpellingRule spellingRule(std::uint8_t rule) noexcept;

} // namespace whitaker::parse
