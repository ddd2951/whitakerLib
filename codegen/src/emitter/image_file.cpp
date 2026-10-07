#include "image_file.hpp"

#include <print>

void emitter::printReport(const ImageFile& image) {
  std::println("  image layout:");
  std::println("  {:<18} {:>10} {:>10} {:>10} {:>10} {:>10}", "object", "start", "end", "bytes", "count", "bits/item");
  for (const ImageFile::Part& part : image.parts)
    std::println("  {:<18} {:>10} {:>10} {:>10} {:>10} {:>10}", part.name, part.start, part.end, part.end - part.start,
                 part.count, part.width * 8);
}
