#include "emitter.hpp"

#include "emitter/image_file.hpp"
#include "emitter/relationship_emitter/image_builder.hpp"
#include "emitter/relationship_emitter/image_writer.hpp"
#include "emitter/relationship_emitter/relationship_index.hpp"
#include "emitter/relationship_emitter/relationship_post_validation.hpp"
#include "error/error.hpp"
#include "publish/atomic_publication.hpp"
#include "src/search/relationship_image.hpp"

#include <filesystem>
#include <print>
#include <string>

namespace fs = std::filesystem;
namespace rel = whitaker::relationship;

namespace {

emitter::ImageFile encode() {
  const emitter::ImageData imageData = emitter::buildImageData();
  const emitter::Machine machine = emitter::buildMachine(imageData.program.words);
  emitter::ImageFile image = emitter::encodeImage(imageData, machine);
  emitter::printReport(image);
  std::println("  {} spellings, {} ordered analyses, {} states, {} transitions", imageData.words.size(),
               imageData.analyses.size(), machine.states.size(), machine.transitions.size());
  return image;
}

} // namespace

void emitter::emit() {
  const fs::path destination{"data/whitaker.dat"};
  if (!fs::is_directory(destination.parent_path()))
    error::fatal("image: no data/ directory here; run gen from the repository root");

  const ImageFile image = encode();
  std::string failure;
  if (!publish::atomically(destination, image.bytes, validateImage, failure))
    error::fatal("image: " + failure);
  std::println("  published {}: {} bytes (format version {})", destination.filename().string(), image.bytes.size(),
               rel::kVersion);
}
