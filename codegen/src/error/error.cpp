#include "error/error.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace error {
namespace {

const char* basename(const char* path) noexcept {
  const char* base = path;
  for (const char* c = path; *c != '\0'; ++c)
    if (*c == '/' || *c == '\\')
      base = c + 1;
  return base;
}

} // namespace

[[noreturn]] void fatal(const char* message,
                        std::source_location location) noexcept {
  std::fprintf(stderr, "[error] %s (%s:%u)\n",
               message == nullptr ? "" : message,
               basename(location.file_name()), location.line());
  std::exit(1);
}

[[noreturn]] void fatal(const std::string& message,
                        std::source_location location) noexcept {
  fatal(message.c_str(), location);
}

} // namespace error
