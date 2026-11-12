// INFO: Writes what is generated from whitaker.hpp's tags: each scheme's files (schemes/: C, Python), and the
//  inspector's API view of all three (web.hpp). Runs from the repository root.
#include "bindings/schemes/c.hpp"
#include "bindings/schemes/python.hpp"
#include "bindings/web.hpp"

#include <cstdio>
#include <span>
#include <string_view>

namespace {

[[nodiscard]] bool write(const char* path, std::string_view text) {
  std::FILE* const file = std::fopen(path, "wb");
  if (file == nullptr) {
    std::perror(path);
    return false;
  }
  const bool written = std::fwrite(text.data(), 1, text.size(), file) == text.size();
  if (std::fclose(file) != 0 || !written) {
    std::perror(path);
    return false;
  }
  return true;
}

} // namespace

int main() {
  const std::span<const bindings::Output> schemes[] = {bindings::python::kOutputs, bindings::c::kOutputs,
                                                       bindings::web::kOutputs};
  for (const auto outputs : schemes)
    for (const bindings::Output& output : outputs)
      if (!write(output.path, output.text))
        return 1;
  return 0;
}
