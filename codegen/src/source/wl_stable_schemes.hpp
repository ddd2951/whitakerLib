#pragma once

// A scheme is one source file's format: its counts, widths, offsets, separators
// and byte rules, each measured on the pinned file and pinned by static_assert.
// Leaves decode against it and hold only the reading; a new corpus is a new
// folder beside wl_stable/, not a change to a leaf.

#include <cstddef>
#include <string_view>

#include "source/wl_stable/addons_scheme.hpp"
#include "source/wl_stable/dictline_scheme.hpp"
#include "source/wl_stable/inflects_scheme.hpp"
#include "source/wl_stable/uniques_scheme.hpp"

namespace source::wl_stable::scheme {

enum class SourceFileKind { dictline, inflects, uniques, addons };

consteval std::size_t getLineCount(SourceFileKind et) {
  switch (et) {
  case SourceFileKind::dictline:
    return dictline::scheme::kLinesPerEntry;
  case SourceFileKind::inflects:
    return inflects::scheme::kLinesPerEntry;
  case SourceFileKind::uniques:
    return uniques::scheme::kLinesPerEntry;
  case SourceFileKind::addons:
    return addons::scheme::kLinesPerEntry;
  }
}

consteval std::size_t getEntryCount(SourceFileKind et) {
  switch (et) {
  case SourceFileKind::dictline:
    return dictline::scheme::kEntriesPerFile;
  case SourceFileKind::inflects:
    return inflects::scheme::kEntriesPerFile;
  case SourceFileKind::uniques:
    return uniques::scheme::kEntriesPerFile;
  case SourceFileKind::addons:
    return addons::scheme::kEntriesPerFile;
  }
}

// The one shape construction reads: how many physical lines the file holds.
// Everything else here describes what a line means, which is tokenization's.
consteval std::size_t getLinesPerFile(SourceFileKind et) {
  switch (et) {
  case SourceFileKind::dictline:
    return dictline::scheme::kLinesPerFile;
  case SourceFileKind::inflects:
    return inflects::scheme::kLinesPerFile;
  case SourceFileKind::uniques:
    return uniques::scheme::kLinesPerFile;
  case SourceFileKind::addons:
    return addons::scheme::kLinesPerFile;
  }
}

// How a source ends a physical line. The formatter does not read this: it ends
// a line on LF and drops a trailing CR, one rule for all four. This is what
// that rule is checked against, so the two are independent.
consteval std::string_view getLineEnding(SourceFileKind et) {
  switch (et) {
  case SourceFileKind::dictline:
    return dictline::scheme::kLineEnding.symbol;
  case SourceFileKind::inflects:
    return inflects::scheme::kLineEnding.symbol;
  case SourceFileKind::uniques:
    return uniques::scheme::kLineEnding.symbol;
  case SourceFileKind::addons:
    return addons::scheme::kLineEnding.symbol;
  }
}

} // namespace source::wl_stable::scheme
