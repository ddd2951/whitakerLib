#pragma once

#include <filesystem>

namespace wl_stable::expand {
struct Forms;
struct Readings;
struct Swept;
} // namespace wl_stable::expand

namespace wl_stable::emitter {

void validatePublishedRelationshipImage(const expand::Forms& forms,
                                        const expand::Readings& readings,
                                        const expand::Swept& swept,
                                        const std::filesystem::path& path);

} // namespace wl_stable::emitter
