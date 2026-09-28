#include "emitter.hpp"

#include <main_config.hpp>

#include "error/error.hpp"
#include "emitter/image_file.hpp"
#include "emitter/relationship_emitter/image_builder.hpp"
#include "emitter/relationship_emitter/image_writer.hpp"
#include "emitter/relationship_emitter/relationship_index.hpp"
#include "emitter/relationship_emitter/relationship_post_validation.hpp"
#include "src/search/relationship_image.hpp"

#include <filesystem>
#include <print>
#include <string>

namespace fs = std::filesystem;
namespace rel = whitaker::relationship;

void emitter::emit() {
  const fs::path destination{main_config::kOutputPath};
  const fs::path directory =
      destination.has_parent_path() ? destination.parent_path() : fs::path{"."};
  if (!fs::is_directory(directory))
    error::fatal("relationship emitter: the destination directory does not "
                 "exist");

  {
    const ImageData imageData = buildImageData();
    const DirectOutputMachine machine(imageData.program.words);
    const ImageFile image = encodeImage(imageData, machine);

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
    std::println(
        "  {} spellings, {} ordered analyses, {} states, {} transitions",
        imageData.words.size(), imageData.analyses.size(), machine.states(),
        machine.transitions());
  }
  std::println("emitter has run");

  validatePublishedRelationshipImage(destination);
  std::println("emitter post has run");
}
