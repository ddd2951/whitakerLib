#pragma once

#include <string_view>

namespace wl_stable::tokenize::internal::dictline {

std::string_view extractSenses(std::string_view rest, std::string_view where);

} // namespace wl_stable::tokenize::internal::dictline
