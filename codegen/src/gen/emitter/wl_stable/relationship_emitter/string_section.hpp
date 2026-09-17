#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace wl_stable::emitter::detail::relationship_image {

// Image String section, every text once, NUL-terminated
// Offset 0 is the empty string
class StringSection {
public:
  StringSection();

  [[nodiscard]] std::uint32_t intern(std::string_view text);
  [[nodiscard]] const std::string& blob() const noexcept;
  [[nodiscard]] std::size_t count() const noexcept;

private:
  std::string m_blob;
  std::unordered_map<std::string, std::uint32_t> m_offsets;
};

} // namespace wl_stable::emitter::detail::relationship_image
