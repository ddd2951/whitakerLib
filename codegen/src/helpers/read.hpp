#pragma once

#include "error/error.hpp"
#include "storage/lines.hpp"
#include "util/file_load.hpp"

#include <cstddef>
#include <meta>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace helpers {

using storage::lines;

template <std::meta::info file> void read() {
  constexpr std::string_view path = [:file:] ::kPath;
  const std::vector<char> bytes = util::fileLoad(path);
  std::size_t next{};
  std::string line{};
  for (const char byte : bytes) {
    if (byte != '\n') {
      line.push_back(byte);
      continue;
    }
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    if (next == lines<file>.size())
      error::invalid("line count over the scheme's " + std::to_string(lines<file>.size()), path);
    lines<file>[next++] = std::move(line);
    line.clear();
  }
  if (!line.empty())
    error::invalid("last line without a terminator", std::string{path} + ":" + std::to_string(next + 1));
  if (next != lines<file>.size())
    error::invalid("line count under the scheme's " + std::to_string(lines<file>.size()), path);
}

} // namespace helpers
