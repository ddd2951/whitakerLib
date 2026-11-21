#include "string_section.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace emitter {

StringSection::StringSection() {
  contents.push_back('\0');
  offsets.emplace("", 0);
}

std::uint32_t StringSection::intern(const std::string_view text) {
  const auto found = offsets.find(std::string{text});
  if (found != offsets.end())
    return found->second;
  assert(contents.size() + text.size() + 1 <= std::numeric_limits<std::uint32_t>::max());
  const std::uint32_t offset = static_cast<std::uint32_t>(contents.size());
  contents.append(text);
  contents.push_back('\0');
  offsets.emplace(std::string{text}, offset);
  return offset;
}

const std::string& StringSection::blob() const { return contents; }

std::size_t StringSection::count() const { return offsets.size(); }

} // namespace emitter
