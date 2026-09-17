#pragma once

#include "error/error.hpp"
#include "source/wl_stable_schemes.hpp"
#include "types/formatted_file.hpp"
#include "util/file_load.hpp"

#include <algorithm>
#include <cstddef>
#include <print>
#include <string>
#include <string_view>
#include <vector>

namespace wl_stable::construction::post_tests {

template <formatted::Kind kind>
void showLine(const formatted::SourceFile<kind>& file, std::size_t number) {
  if (number == 0 || number > file.lines.size())
    error::fatal(file.path + ":" + std::to_string(number) +
                 ": no such line, the file holds " +
                 std::to_string(file.lines.size()));
  std::println("{}:{}: |{}|", file.path, number, file.lines[number - 1]);
}

// Prints the first and last line of each source, and the count.
inline void reportFormattedLedger(const formatted::Sources& sources) {
  const auto report = [](const auto& file) {
    std::println("formatted lines: {} ({})", file.path, file.lines.size());
    showLine(file, 1);
    showLine(file, file.lines.size());
  };

  report(sources.dictline);
  report(sources.inflects);
  report(sources.uniques);
  report(sources.addons);
}

inline void verifyNoTerminators(const formatted::Sources& sources) {
  const auto verify = [](const auto& file) {
    std::size_t number{1};
    for (const std::string& line : file.lines) {
      if (line.contains('\r') || line.contains('\n'))
        error::fatal(file.path + ":" + std::to_string(number) +
                     ": formatted line holds a line ending");
      ++number;
    }
  };

  verify(sources.dictline);
  verify(sources.inflects);
  verify(sources.uniques);
  verify(sources.addons);
}

// HACK: This is our expensive byte for byte post validation.
// Currently it's expensive and dumb, but it's to ensure it's not just doing
// what the construction did a second time.
// It stays a HACK because the honest version needs an independent authority
// for what these files hold, and we do not have one at this point.
template <formatted::Kind kind>
void verifyRoundTrip(const formatted::SourceFile<kind>& file) {
  constexpr std::string_view ending =
      source::wl_stable::scheme::getLineEnding(kind);

  std::string rebuilt;
  std::size_t size{};
  for (const std::string& line : file.lines)
    size += line.size() + ending.size();
  rebuilt.reserve(size);
  for (const std::string& line : file.lines) {
    rebuilt += line;
    rebuilt += ending;
  }
  const auto lineHolding = [&file, ending](std::size_t offset) {
    std::size_t start{};
    std::size_t number{1};
    for (const std::string& line : file.lines) {
      start += line.size() + ending.size();
      if (offset < start)
        break;
      ++number;
    }
    return number;
  };
  const std::vector<char> source = util::fileLoad(file.path);
  const std::size_t shared = std::min(rebuilt.size(), source.size());
  for (std::size_t at{}; at < shared; ++at)
    if (rebuilt[at] != source[at])
      error::fatal(file.path + ":" + std::to_string(lineHolding(at)) +
                   ": rebuilt source differs at byte " + std::to_string(at));
  if (rebuilt.size() != source.size())
    error::fatal(file.path + ": rebuilt source is " +
                 std::to_string(rebuilt.size()) + " bytes, the file is " +
                 std::to_string(source.size()));
}

inline void verifyFormattedRoundTrip(const formatted::Sources& sources) {
  verifyRoundTrip(sources.dictline);
  verifyRoundTrip(sources.inflects);
  verifyRoundTrip(sources.uniques);
  verifyRoundTrip(sources.addons);
}

} // namespace wl_stable::construction::post_tests
