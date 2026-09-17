#pragma once

#include <string_view>

namespace wl_stable::tokenize::internal::addons {

std::string_view extractMeaning(std::string_view line, std::string_view where);

} // namespace wl_stable::tokenize::internal::addons
