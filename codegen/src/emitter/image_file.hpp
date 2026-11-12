#pragma once

#include <cstddef>
#include <string_view>
#include <vector>
#include <type_traits>

namespace emitter {

// INFO: Image bytes and section locations for the layout report.
struct ImageFile {
  struct Part {
    std::string_view name{};
    std::size_t start{};
    std::size_t end{};
    std::size_t count{};
    std::size_t width{};
  };
  static_assert(std::is_trivially_copyable_v<Part>);
  std::vector<unsigned char> bytes{};
  std::vector<Part> parts{};
};

void printReport(const ImageFile& image);

} // namespace emitter
