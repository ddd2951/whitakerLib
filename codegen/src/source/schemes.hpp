#pragma once

#include <cstddef>
#include <string_view>

#include "source/addons_scheme.hpp"
#include "source/dictline_scheme.hpp"
#include "source/inflects_scheme.hpp"
#include "source/uniques_scheme.hpp"

namespace source::scheme {

enum class SourceFileKind { dictline, inflects, uniques, addons };

consteval std::size_t getLineCount(SourceFileKind et) {
  switch (et) {
  case SourceFileKind::dictline:
    return dictline::kLinesPerEntry;
  case SourceFileKind::inflects:
    return inflects::kLinesPerEntry;
  case SourceFileKind::uniques:
    return uniques::kLinesPerEntry;
  case SourceFileKind::addons:
    return addons::kLinesPerEntry;
  }
}

consteval std::size_t getEntryCount(SourceFileKind et) {
  switch (et) {
  case SourceFileKind::dictline:
    return dictline::kEntriesPerFile;
  case SourceFileKind::inflects:
    return inflects::kEntriesPerFile;
  case SourceFileKind::uniques:
    return uniques::kEntriesPerFile;
  case SourceFileKind::addons:
    return addons::kEntriesPerFile;
  }
}

// NOTE: Source frames entries; word interprets them.
consteval std::size_t getLinesPerFile(SourceFileKind et) {
  switch (et) {
  case SourceFileKind::dictline:
    return dictline::kLinesPerFile;
  case SourceFileKind::inflects:
    return inflects::kLinesPerFile;
  case SourceFileKind::uniques:
    return uniques::kLinesPerFile;
  case SourceFileKind::addons:
    return addons::kLinesPerFile;
  }
}

// NOTE: Reading accepts LF or CRLF; verification checks the pinned ending.
consteval std::string_view getLineEnding(SourceFileKind et) {
  switch (et) {
  case SourceFileKind::dictline:
    return dictline::kLineEnding.symbol;
  case SourceFileKind::inflects:
    return inflects::kLineEnding.symbol;
  case SourceFileKind::uniques:
    return uniques::kLineEnding.symbol;
  case SourceFileKind::addons:
    return addons::kLineEnding.symbol;
  }
}

} // namespace source::scheme
