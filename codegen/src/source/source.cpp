#include "source.hpp"

#include "error/error.hpp"
#include "source/schemes.hpp"
#include "util/file_load.hpp"

#include <array>
#include <cstddef>
#include <meta>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace scheme = source::scheme;

template <std::meta::info file> std::array<std::string, [:file:] ::kLinesPerFile> lines;

template <std::meta::info file> void read() {
  constexpr std::string_view path = [:file:] ::kPath;
  const std::vector<char> bytes = util::fileLoad(path);
  std::size_t next{};
  std::string line;
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

template <std::meta::info file> std::array<std::size_t, [:file:] ::kEntriesPerFile * [:file:] ::kLinesPerEntry> records;

template <std::meta::info file> void indexEveryLine() {
  static_assert(records<file>.size() == lines<file>.size());
  for (std::size_t at{}; at < records<file>.size(); ++at)
    records<file>[at] = at;
}

// NOTE: ADDONS meanings can contain `--`, so framing skips only whole-line comments.
template <std::meta::info file> void frame() {
  constexpr std::string_view path = [:file:] ::kPath;
  constexpr std::string_view commentMarker = [:file:] ::kCommentMarker;
  constexpr std::string_view whitespace = [:file:] ::kWhitespace;
  std::size_t index{};
  for (std::size_t at{}; at < lines<file>.size(); ++at) {
    const std::string_view text = lines<file>[at];
    const std::size_t content = text.find_first_not_of(whitespace);
    if (content == std::string_view::npos)
      continue;
    if (text.substr(content).starts_with(commentMarker))
      continue;
    if (index == records<file>.size())
      error::invalid("record line past the scheme's " + std::to_string(records<file>.size()),
                     std::string{path} + ":" + std::to_string(at + 1));
    records<file>[index++] = at;
  }
  if (index != records<file>.size())
    error::invalid(std::to_string(index) + " record lines, the scheme's " + std::to_string(records<file>.size()), path);
}

template <std::meta::info file> std::string_view recordLine(std::size_t entry, std::size_t part) {
  return lines<file>[records<file>[entry * [:file:] ::kLinesPerEntry + part]];
}

template <std::meta::info file> std::size_t firstLineNumber(std::size_t entry) {
  return records<file>[entry * [:file:] ::kLinesPerEntry] + 1;
}

} // namespace

void source::init() {
  read<^^scheme::dictline>();
  read<^^scheme::inflects>();
  read<^^scheme::uniques>();
  read<^^scheme::addons>();
  indexEveryLine<^^scheme::dictline>();
  frame<^^scheme::inflects>();
  indexEveryLine<^^scheme::uniques>();
  frame<^^scheme::addons>();
}

std::string_view source::dictline(DictlineIndex entry) { return recordLine<^^scheme::dictline>(entry.value, 0); }
std::string_view source::inflects(InflectsIndex entry) { return recordLine<^^scheme::inflects>(entry.value, 0); }
source::UniquesLines source::uniques(UniquesIndex entry) {
  return {recordLine<^^scheme::uniques>(entry.value, 0), recordLine<^^scheme::uniques>(entry.value, 1),
          recordLine<^^scheme::uniques>(entry.value, 2)};
}
source::AddonsLines source::addons(AddonsIndex entry) {
  return {recordLine<^^scheme::addons>(entry.value, 0), recordLine<^^scheme::addons>(entry.value, 1),
          recordLine<^^scheme::addons>(entry.value, 2)};
}

std::size_t source::lineNumber(DictlineIndex entry) { return firstLineNumber<^^scheme::dictline>(entry.value); }
std::size_t source::lineNumber(InflectsIndex entry) { return firstLineNumber<^^scheme::inflects>(entry.value); }
std::size_t source::lineNumber(UniquesIndex entry) { return firstLineNumber<^^scheme::uniques>(entry.value); }
std::size_t source::lineNumber(AddonsIndex entry) { return firstLineNumber<^^scheme::addons>(entry.value); }
