#pragma once

#include <string_view>

namespace wl_stable::tokenize::internal::addons {

enum class Kind { prefix, suffix, tackon };

struct Fix {
  Kind kind;
  std::string_view text;
  char connect; // '\0' when none
};

Fix extractFix(std::string_view line, std::string_view where);

} // namespace wl_stable::tokenize::internal::addons
