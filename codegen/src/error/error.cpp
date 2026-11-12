#include "error/error.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace error {
namespace {

const char* basename(const char* const path) {
  const char* base = path;
  for (const char* c = path; *c != '\0'; ++c)
    if (*c == '/' || *c == '\\')
      base = c + 1;
  return base;
}

} // namespace

[[noreturn]] void fatal(const char* const message, const std::source_location location) {
  std::fprintf(stderr, "[error] %s (%s:%u)\n", message == nullptr ? "" : message, basename(location.file_name()),
               location.line());
  std::exit(1);
}

[[noreturn]] void fatal(const std::string& message, const std::source_location location) {
  fatal(message.c_str(), location);
}

[[noreturn]] void invalid(const std::string_view what, const std::string_view where,
                          const std::source_location location) {
  std::string message = "Invalid: ";
  message += what;
  if (!where.empty())
    message.append(" at ").append(where);
  fatal(message, location);
}

} // namespace error
