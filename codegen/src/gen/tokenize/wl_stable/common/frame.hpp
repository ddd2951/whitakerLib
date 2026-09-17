#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "error/error.hpp"
#include "source/wl_stable_schemes.hpp"
#include "types/formatted_file.hpp"

namespace wl_stable::tokenize::internal {

// Cutting a trailing comment is the source's reading, not framing's: ADDONS
// meanings contain `--`.
template <formatted::Kind kind, typename Record>
void frame(const formatted::SourceFile<kind>& file,
           std::string_view commentMarker, std::string_view whitespace,
           Record&& record) {
  constexpr std::size_t kExpected =
      source::wl_stable::scheme::getEntryCount(kind) *
      source::wl_stable::scheme::getLineCount(kind);
  std::size_t index = 0;
  for (std::size_t at = 0; at < file.lines.size(); ++at) {
    const std::string_view line = file.lines[at];
    const std::size_t content = line.find_first_not_of(whitespace);
    if (content == std::string_view::npos)
      continue;
    if (line.substr(content).starts_with(commentMarker))
      continue;
    if (index == kExpected)
      error::fatal(file.path + ":" + std::to_string(at + 1) +
                   ": more record lines than the scheme's " +
                   std::to_string(kExpected));
    record(index++, at + 1, line);
  }
  if (index != kExpected)
    error::fatal(file.path + ": " + std::to_string(index) +
                 " record lines, the scheme states " +
                 std::to_string(kExpected));
}

} // namespace wl_stable::tokenize::internal
