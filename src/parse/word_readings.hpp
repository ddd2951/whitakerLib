#pragma once

#include "parse/result.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace whitaker::parse {

enum class Stage : std::uint8_t { Dictionary, Affixes };

inline constexpr std::size_t kEncliticTackons = 4;

[[nodiscard]] std::string_view withoutTackon(std::string_view word, const rel::Addon& tackon) noexcept;
void addWordReadings(Work& out, Text& text, std::string_view word, Stage stage) noexcept;

} // namespace whitaker::parse
