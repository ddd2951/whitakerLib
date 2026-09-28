#include "source.hpp"

#include <main_config.hpp>

#include "error/error.hpp"
#include "source/schemes.hpp"
#include "util/file_load.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <print>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace scheme = source::scheme;
using Kind = scheme::SourceFileKind;

// NOTE: No two paths may name one file. Added after it happened.
static_assert(main_config::kSourceDictlinePath !=
                  main_config::kSourceInflectsPath,
              "DICTLINE.GEN and INFLECTS.LAT are the same path");
static_assert(main_config::kSourceDictlinePath !=
                  main_config::kSourceUniquesPath,
              "DICTLINE.GEN and UNIQUES.LAT are the same path");
static_assert(main_config::kSourceDictlinePath !=
                  main_config::kSourceAddonsPath,
              "DICTLINE.GEN and ADDONS.LAT are the same path");
static_assert(main_config::kSourceInflectsPath !=
                  main_config::kSourceUniquesPath,
              "INFLECTS.LAT and UNIQUES.LAT are the same path");
static_assert(main_config::kSourceInflectsPath !=
                  main_config::kSourceAddonsPath,
              "INFLECTS.LAT and ADDONS.LAT are the same path");
static_assert(main_config::kSourceUniquesPath != main_config::kSourceAddonsPath,
              "UNIQUES.LAT and ADDONS.LAT are the same path");

consteval std::string_view getPath(Kind kind) {
  switch (kind) {
  case Kind::dictline:
    return main_config::kSourceDictlinePath;
  case Kind::inflects:
    return main_config::kSourceInflectsPath;
  case Kind::uniques:
    return main_config::kSourceUniquesPath;
  case Kind::addons:
    return main_config::kSourceAddonsPath;
  }
}

constexpr char kLineFeed{'\n'};
constexpr char kCarriageReturn{'\r'};

template <Kind kind>
std::array<std::string, scheme::getLinesPerFile(kind)> lines;

template <Kind kind> void read() {
  constexpr std::string_view path = getPath(kind);
  const std::vector<char> bytes = util::fileLoad(path);
  std::size_t next{};
  std::string line;
  for (const char byte : bytes) {
    if (byte != kLineFeed) {
      line.push_back(byte);
      continue;
    }
    if (!line.empty() && line.back() == kCarriageReturn)
      line.pop_back();
    if (next == lines<kind>.size())
      error::fatal(std::string{path} + ": holds more lines than the " +
                   std::to_string(lines<kind>.size()) + " its scheme states");
    lines<kind>[next++] = std::move(line);
    line.clear();
  }
  if (!line.empty())
    error::fatal(std::string{path} + ":" + std::to_string(next + 1) +
                 ": last line has no terminator");
  if (next != lines<kind>.size())
    error::fatal(std::string{path} + ": holds " + std::to_string(next) +
                 " lines, its scheme states " +
                 std::to_string(lines<kind>.size()));
}

template <Kind kind> void verify() {
  constexpr std::string_view path = getPath(kind);
  constexpr std::string_view ending = scheme::getLineEnding(kind);
  const auto& file = lines<kind>;

  std::println("formatted lines: {} ({})", path, file.size());
  std::println("{}:{}: |{}|", path, 1, file.front());
  std::println("{}:{}: |{}|", path, file.size(), file.back());

  std::size_t number{1};
  for (const std::string& line : file) {
    if (line.contains('\r') || line.contains('\n'))
      error::fatal(std::string{path} + ":" + std::to_string(number) +
                   ": formatted line holds a line ending");
    ++number;
  }

  // NOTE: Compare every source byte, including the pinned line endings.
  std::string rebuilt;
  std::size_t size{};
  for (const std::string& line : file)
    size += line.size() + ending.size();
  rebuilt.reserve(size);
  for (const std::string& line : file) {
    rebuilt += line;
    rebuilt += ending;
  }
  const auto lineHolding = [&file, ending](std::size_t offset) {
    std::size_t start{};
    std::size_t holding{1};
    for (const std::string& line : file) {
      start += line.size() + ending.size();
      if (offset < start)
        break;
      ++holding;
    }
    return holding;
  };
  const std::vector<char> bytes = util::fileLoad(path);
  const std::size_t shared = std::min(rebuilt.size(), bytes.size());
  for (std::size_t at{}; at < shared; ++at)
    if (rebuilt[at] != bytes[at])
      error::fatal(std::string{path} + ":" + std::to_string(lineHolding(at)) +
                   ": rebuilt source differs at byte " + std::to_string(at));
  if (rebuilt.size() != bytes.size())
    error::fatal(std::string{path} + ": rebuilt source is " +
                 std::to_string(rebuilt.size()) + " bytes, the file is " +
                 std::to_string(bytes.size()));
}

template <Kind kind>
std::array<std::size_t,
           scheme::getEntryCount(kind) * scheme::getLineCount(kind)>
    records;

template <Kind kind> void everyLine() {
  static_assert(records<kind>.size() == lines<kind>.size());
  for (std::size_t at{}; at < records<kind>.size(); ++at)
    records<kind>[at] = at;
}

// NOTE: ADDONS meanings can contain `--`, so framing skips only whole-line
//  comments.
template <Kind kind>
void frame(std::string_view commentMarker, std::string_view whitespace) {
  constexpr std::string_view path = getPath(kind);
  std::size_t index{};
  for (std::size_t at{}; at < lines<kind>.size(); ++at) {
    const std::string_view text = lines<kind>[at];
    const std::size_t content = text.find_first_not_of(whitespace);
    if (content == std::string_view::npos)
      continue;
    if (text.substr(content).starts_with(commentMarker))
      continue;
    if (index == records<kind>.size())
      error::fatal(std::string{path} + ":" + std::to_string(at + 1) +
                   ": more record lines than the scheme's " +
                   std::to_string(records<kind>.size()));
    records<kind>[index++] = at;
  }
  if (index != records<kind>.size())
    error::fatal(std::string{path} + ": " + std::to_string(index) +
                 " record lines, the scheme states " +
                 std::to_string(records<kind>.size()));
}

template <Kind kind>
std::string_view recordLine(std::size_t entry, std::size_t part) {
  return lines<kind>[records<kind>[entry * scheme::getLineCount(kind) + part]];
}

template <Kind kind> std::size_t firstLineNumber(std::size_t entry) {
  return records<kind>[entry * scheme::getLineCount(kind)] + 1;
}

} // namespace

void source::init() {
  read<Kind::dictline>();
  read<Kind::inflects>();
  read<Kind::uniques>();
  read<Kind::addons>();
  verify<Kind::dictline>();
  verify<Kind::inflects>();
  verify<Kind::uniques>();
  verify<Kind::addons>();
  everyLine<Kind::dictline>();
  frame<Kind::inflects>(scheme::inflects::kCommentMarker,
                        scheme::inflects::kWhitespace);
  everyLine<Kind::uniques>();
  frame<Kind::addons>(scheme::addons::kCommentMarker,
                      scheme::addons::kWhitespace);
}

std::string_view source::dictline(DictlineIndex entry) {
  return recordLine<Kind::dictline>(entry.value, 0);
}
std::string_view source::inflects(InflectsIndex entry) {
  return recordLine<Kind::inflects>(entry.value, 0);
}
source::UniquesLines source::uniques(UniquesIndex entry) {
  return {recordLine<Kind::uniques>(entry.value, 0),
          recordLine<Kind::uniques>(entry.value, 1),
          recordLine<Kind::uniques>(entry.value, 2)};
}
source::AddonsLines source::addons(AddonsIndex entry) {
  return {recordLine<Kind::addons>(entry.value, 0),
          recordLine<Kind::addons>(entry.value, 1),
          recordLine<Kind::addons>(entry.value, 2)};
}

std::size_t source::lineNumber(DictlineIndex entry) {
  return firstLineNumber<Kind::dictline>(entry.value);
}
std::size_t source::lineNumber(InflectsIndex entry) {
  return firstLineNumber<Kind::inflects>(entry.value);
}
std::size_t source::lineNumber(UniquesIndex entry) {
  return firstLineNumber<Kind::uniques>(entry.value);
}
std::size_t source::lineNumber(AddonsIndex entry) {
  return firstLineNumber<Kind::addons>(entry.value);
}
