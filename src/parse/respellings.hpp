#pragma once

#include "parse/result.hpp"

#include <cstdint>
#include <string_view>

namespace whitaker::parse {

bool addPrefixVariantReadings(Work& out, std::string_view input);
bool addRespelledReadings(Work& out, std::string_view input);
void addRespelledEncliticReadings(Work& out, std::string_view word);
[[nodiscard]] SpellingRule spellingRule(std::uint8_t rule);

} // namespace whitaker::parse
