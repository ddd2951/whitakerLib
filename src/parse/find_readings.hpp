#pragma once

#include "parse/result.hpp"

#include <string_view>

namespace whitaker::parse {

void addSyncopeReadings(Work& out, Text& text, std::string_view word) noexcept;
void findReadings(Work& out, Text& text, std::string_view word) noexcept;

} // namespace whitaker::parse
