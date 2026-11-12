#pragma once

#include "parse/result.hpp"

#include <string_view>

namespace whitaker::parse {

void addSyncopeReadings(Work& out, std::string_view word);
void findReadings(Work& out, std::string_view word);

} // namespace whitaker::parse
