#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace emitter {

// The image's bytes, and where each part of it went, for the report.
struct ImageFile {
  struct Part {
    std::string_view name;
    std::size_t start{};
    std::size_t end{};
    std::size_t count{};
    std::size_t width{};
  };
  std::vector<unsigned char> bytes;
  std::vector<Part> parts;
};

void printReport(const ImageFile& image);

} // namespace emitter
