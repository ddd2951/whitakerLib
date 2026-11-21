#pragma once

#include "error/error.hpp"
#include "storage/lines.hpp"
#include "storage/records.hpp"

#include <cstddef>
#include <meta>
#include <string>
#include <string_view>

namespace helpers {

// NOTE: ADDONS meanings can contain `--`, so framing skips only whole-line comments.
template <std::meta::info file> void index() {
  using storage::lines;
  using storage::records;
  constexpr std::string_view path = [:file:] ::kPath;
  constexpr std::string_view commentMarker = [:file:] ::kCommentMarker;
  constexpr std::string_view whitespace = [:file:] ::kWhitespace;
  std::size_t next{};
  for (std::size_t at{}; at < lines<file>.size(); ++at) {
    const std::string_view text = lines<file>[at];
    const std::size_t content = text.find_first_not_of(whitespace);
    if (content == std::string_view::npos)
      continue;
    if (text.substr(content).starts_with(commentMarker))
      continue;
    if (next == records<file>.size())
      error::invalid("record line past the scheme's " + std::to_string(records<file>.size()),
                     std::string{path} + ":" + std::to_string(at + 1));
    records<file>[next++] = at;
  }
  if (next != records<file>.size())
    error::invalid(std::to_string(next) + " record lines, the scheme's " + std::to_string(records<file>.size()), path);
}

} // namespace helpers
