#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace emitter {

// INFO: The image's string section: every text once, NUL-terminated. Offset 0 is the empty string.
class StringSection {
public:
  StringSection();

  [[nodiscard]] std::uint32_t intern(std::string_view text);
  [[nodiscard]] const std::string& blob() const;
  [[nodiscard]] std::size_t count() const;

private:
  std::string contents{};
  std::unordered_map<std::string, std::uint32_t> offsets{};
};

} // namespace emitter
