#pragma once

#include <cstddef>
#include <functional>
#include <string_view>

#include "types/string_pools.hpp"

namespace wl_stable::tokenize::internal {

inline std::string_view place(std::string_view text, text::Pool& pool) {
  const std::size_t at = pool.size();
  pool.append(text);
  return {pool.data() + at, text.size()};
}

inline bool inside(std::string_view view, const text::Pool& pool) {
  const std::less_equal<const char*> le;
  return le(pool.data(), view.data()) &&
         le(view.data() + view.size(), pool.data() + pool.size());
}

} // namespace wl_stable::tokenize::internal
