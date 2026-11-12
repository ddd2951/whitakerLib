#pragma once

#include <source_location>
#include <string>
#include <string_view>

namespace error {

[[noreturn]] void fatal(const char* message, std::source_location location = std::source_location::current());

[[noreturn]] void fatal(const std::string& message, std::source_location location = std::source_location::current());

[[noreturn]] void invalid(std::string_view what, std::string_view where = {},
                          std::source_location location = std::source_location::current());

} // namespace error
