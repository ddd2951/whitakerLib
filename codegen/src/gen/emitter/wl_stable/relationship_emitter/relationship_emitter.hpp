#pragma once

#include "types/tokenized_sources.hpp"

#include <filesystem>

namespace wl_stable::expand {
struct Synthetics;
struct Forms;
struct Readings;
struct Swept;
} // namespace wl_stable::expand

namespace wl_stable::emitter {

void emit(const tokenized::Sources& sources,
          const wl_stable::expand::Synthetics& synthetics,
          const wl_stable::expand::Forms& forms,
          const wl_stable::expand::Readings& readings,
          const wl_stable::expand::Swept& swept,
          const std::filesystem::path& destination);

} // namespace wl_stable::emitter
