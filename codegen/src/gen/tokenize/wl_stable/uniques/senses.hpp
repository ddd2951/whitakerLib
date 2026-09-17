#pragma once

#include <string_view>

namespace wl_stable::tokenize::internal::uniques {

std::string_view extractSenses(std::string_view meaning,
                               std::string_view where);

} // namespace wl_stable::tokenize::internal::uniques
