#pragma once

#include <string_view>

#include "types/types.hpp"

namespace wl_stable::tokenize::internal::uniques {

PosTokenTypes extractPart(std::string_view token, std::string_view where);

} // namespace wl_stable::tokenize::internal::uniques
