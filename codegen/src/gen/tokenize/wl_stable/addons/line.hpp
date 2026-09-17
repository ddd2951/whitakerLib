#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "source/wl_stable/addons_scheme.hpp"
#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::tokenize::internal::addons {

using Entry = std::array<std::string_view,
                         source::wl_stable::addons::scheme::kLinesPerEntry>;

void line(const Entry& entry, std::size_t index, std::string_view where,
          text::StringPools& pools, tokenized::Addons& into);

} // namespace wl_stable::tokenize::internal::addons
