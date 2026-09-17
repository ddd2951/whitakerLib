#pragma once

#include "gen/roster_tag.hpp"
#include "types/tokenized_sources.hpp"

#include <filesystem>

namespace wl_stable::expand {
struct Synthetics;
struct Forms;
struct Readings;
struct Swept;
} // namespace wl_stable::expand

namespace wl_stable {
namespace[[= gen::roster::RosterTag::WlStableEmitter]] emitter {
[[= gen::roster::HookTag::Run]] void
run(const tokenized::Sources& sources, const expand::Synthetics& synthetics,
    const expand::Forms& forms, const expand::Readings& readings,
    const expand::Swept& swept, const std::filesystem::path& destination);
[[= gen::roster::HookTag::Post]] void
post(const expand::Forms& forms, const expand::Readings& readings,
     const expand::Swept& swept, const std::filesystem::path& destination);
} // namespace emitter
} // namespace wl_stable
