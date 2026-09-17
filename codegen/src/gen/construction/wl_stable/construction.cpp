#include "construction.hpp"

#include "error/error.hpp"
#include "gen/construction/config.hpp"
#include "gen/construction/wl_stable/construction_post_tests.hpp"
#include "util/file_load.hpp"

#include <cstddef>
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace construction::config;

namespace {
// Sources end a line with LF or CRLF, construction is the last part to know
// about this difference in source encoding
constexpr char kLineFeed{'\n'};
constexpr char kCarriageReturn{'\r'};

template <formatted::Kind kind>
[[nodiscard]] formatted::SourceFile<kind> formatSource() {
  constexpr std::string_view path = getPath(kind);
  const std::vector<char> bytes = util::fileLoad(path);
  formatted::SourceFile<kind> file{.path = std::string{path}, .lines = {}};
  std::size_t next{};
  std::string line;
  for (const char byte : bytes) {
    if (byte != kLineFeed) {
      line.push_back(byte);
      continue;
    }
    if (!line.empty() && line.back() == kCarriageReturn)
      line.pop_back();
    if (next == file.lines.size())
      error::fatal(std::string{path} + ": holds more lines than the " +
                   std::to_string(file.lines.size()) + " its scheme states");
    file.lines[next++] = std::move(line);
    line.clear();
  }
  if (!line.empty())
    error::fatal(std::string{path} + ":" + std::to_string(next + 1) +
                 ": last line has no terminator");
  if (next != file.lines.size())
    error::fatal(std::string{path} + ": holds " + std::to_string(next) +
                 " lines, its scheme states " +
                 std::to_string(file.lines.size()));

  return file;
}

} // namespace

namespace wl_stable::construction {

formatted::Sources run() {
  formatted::Sources sources{};
  sources.dictline = formatSource<formatted::Kind::dictline>();
  sources.inflects = formatSource<formatted::Kind::inflects>();
  sources.uniques = formatSource<formatted::Kind::uniques>();
  sources.addons = formatSource<formatted::Kind::addons>();

  std::println("wl_stable construction has run");
  return sources;
}

void post(const formatted::Sources& sources) {
  post_tests::reportFormattedLedger(sources);
  post_tests::verifyNoTerminators(sources);
  post_tests::verifyFormattedRoundTrip(sources);
  std::println("wl_stable construction post has run");
}

} // namespace wl_stable::construction
