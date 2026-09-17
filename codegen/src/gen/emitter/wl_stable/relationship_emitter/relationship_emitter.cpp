#include "relationship_emitter.hpp"

#include "error/error.hpp"
#include "gen/emitter/wl_stable/image_file.hpp"
#include "image_builder.hpp"
#include "image_writer.hpp"
#include "relationship_index.hpp"
#include "src/search/relationship_image.hpp"

#include <filesystem>
#include <print>
#include <string>

namespace fs = std::filesystem;
namespace rel = whitaker::relationship;
namespace relationship_image = wl_stable::emitter::detail::relationship_image;
namespace relationship_index = wl_stable::emitter::detail::relationship_index;
using wl_stable::emitter::ImageFile;

void wl_stable::emitter::emit(const tokenized::Sources& sources,
                              const wl_stable::expand::Synthetics& synthetics,
                              const wl_stable::expand::Forms& forms,
                              const wl_stable::expand::Readings& readings,
                              const wl_stable::expand::Swept& swept,
                              const fs::path& destination) {
  const fs::path directory =
      destination.has_parent_path() ? destination.parent_path() : fs::path{"."};
  if (!fs::is_directory(directory))
    error::fatal("relationship emitter: the destination directory does not "
                 "exist");

  const relationship_image::ImageData imageData =
      relationship_image::buildImageData(sources, synthetics, forms, readings,
                                         swept);
  const relationship_index::DirectOutputMachine machine(
      imageData.program.words);
  const ImageFile image = relationship_image::encodeImage(imageData, machine);

  std::string failure;
  const bool published = image.publish(
      destination,
      [](const fs::path& temporary, std::string& message) {
        rel::Image candidate;
        return candidate.load(temporary, message);
      },
      failure);
  if (!published)
    error::fatal(("relationship emitter: " + failure).c_str());

  image.printReport();
  std::println("  published {}: {} bytes (format version {})",
               destination.filename().string(), image.byteSize(),
               rel::kVersion);
  std::println("  {} spellings, {} ordered analyses, {} states, {} transitions",
               imageData.words.size(), imageData.analyses.size(),
               machine.states(), machine.transitions());
}
