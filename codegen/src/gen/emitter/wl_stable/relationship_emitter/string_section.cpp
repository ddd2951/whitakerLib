#include "string_section.hpp"

#include "error/error.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace wl_stable::emitter::detail::relationship_image {

StringSection::StringSection() {
  m_blob.push_back('\0');
  m_offsets.emplace("", 0);
}

std::uint32_t StringSection::intern(std::string_view text) {
  const auto found = m_offsets.find(std::string{text});
  if (found != m_offsets.end())
    return found->second;
  if (m_blob.size() + text.size() + 1 >
      std::numeric_limits<std::uint32_t>::max())
    error::fatal("relationship emitter: string section exceeds uint32_t");
  const std::uint32_t offset = static_cast<std::uint32_t>(m_blob.size());
  m_blob.append(text);
  m_blob.push_back('\0');
  m_offsets.emplace(std::string{text}, offset);
  return offset;
}

const std::string& StringSection::blob() const noexcept { return m_blob; }

std::size_t StringSection::count() const noexcept { return m_offsets.size(); }

} // namespace wl_stable::emitter::detail::relationship_image
