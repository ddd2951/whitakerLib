#include "readings/tables.hpp"
#include "emitter/image_file.hpp"
#include "execution_steps/run.hpp"
#include "emitter/relationship_emitter/image_writer.hpp"
#include "storage/image.hpp"
#include <fstream>
#include <iostream>
int main(const int argc, char** const argv) {
  if (argc != 2)
    return 2;
  runner::run<^^execution_steps::dictline, ^^execution_steps::inflects, ^^execution_steps::uniques,
              ^^execution_steps::addons>();
  constexpr const char* path = "src/search/two_ended_bounds.hpp";
  if (std::string_view(argv[1]) == "--check") {
    runner::run<^^execution_steps::candidates, ^^execution_steps::expansion, ^^execution_steps::image_data,
                ^^execution_steps::lookup_tables, ^^execution_steps::finish_image_data>();
    const auto& data = storage::image;
    std::ifstream in(path, std::ios::binary);
    const std::string written((std::istreambuf_iterator<char>(in)), {});
    if (written != data.twoEnded.metadata) {
      std::cerr << path << " is stale: build write_two_ended\n";
      return 1;
    }
    const auto expected = emitter::encodeImage(data);
    std::ifstream image("data/whitaker.dat", std::ios::binary);
    const std::vector<unsigned char> stored((std::istreambuf_iterator<char>(image)), {});
    if (stored != expected.bytes) {
      std::cerr << "data/whitaker.dat is stale: build image\n";
      return 1;
    }
  } else if (std::string_view(argv[1]) == "--write") {
    std::ofstream out(path, std::ios::binary);
    runner::run<^^execution_steps::candidates, ^^execution_steps::expansion, ^^execution_steps::image_data,
                ^^execution_steps::lookup_tables, ^^execution_steps::finish_image_data>();
    const auto& data = storage::image;
    out << data.twoEnded.metadata;
    if (!out)
      return 1;
  } else
    return 2;
}
