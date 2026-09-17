#include "emitter.hpp"

#include "gen/emitter/wl_stable/relationship_emitter/relationship_emitter.hpp"
#include "gen/emitter/wl_stable/relationship_emitter/relationship_post_validation.hpp"

#include <print>

void wl_stable::emitter::run(const tokenized::Sources& sources,
                             const expand::Synthetics& synthetics,
                             const expand::Forms& forms,
                             const expand::Readings& readings,
                             const expand::Swept& swept,
                             const std::filesystem::path& destination) {
  emit(sources, synthetics, forms, readings, swept, destination);
  std::println("wl_stable emitter has run");
}

void wl_stable::emitter::post(const expand::Forms& forms,
                              const expand::Readings& readings,
                              const expand::Swept& swept,
                              const std::filesystem::path& destination) {
  validatePublishedRelationshipImage(forms, readings, swept, destination);
  std::println("wl_stable emitter post has run");
}
