#pragma once

#include <string_view>

namespace whitaker {

// Returns 0 for non-numerals. The accepted historical forms intentionally
// include IIII and VIIII for compatibility with Whitaker's Words.
[[nodiscard]] unsigned romanValue(std::string_view word) noexcept;

} // namespace whitaker
