#pragma once

#include <span>
#include <string_view>

#include "types/types.hpp"
#include "util/reflect_util.hpp"

namespace wl_stable::tokenize::internal::pre_tests {

consteval bool partsAreNamed(std::span<const std::string_view> parts) {
  for (const std::string_view part : parts)
    if (!util::trySvToEnum<PosTokenTypes>(part))
      return false;
  return true;
}

// Every scheme's kParts is proved against PosTokenTypes when this file's
// .cpp compiles; the function only reports that.
void verifySchemeParts();

} // namespace wl_stable::tokenize::internal::pre_tests
