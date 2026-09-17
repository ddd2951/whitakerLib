#pragma once

#include <array>
#include <string>

#include "source/wl_stable_schemes.hpp"

// Construction's handoff: one box per physical source line, in file order,
// holding that line's bytes exactly as they sit on disk minus the terminator
// that ended it. Construction asks nothing else about a line. Comments, blank
// lines and multi-line records are all judgements about what a line means, and
// tokenization is the first layer allowed to make them.
namespace formatted {

// Which source a formatted file came from. It is part of this type's
// interface, so it is named here rather than spelled out at every use.
using Kind = source::wl_stable::scheme::SourceFileKind;

// The array is sized from the scheme, so a source that gains or loses a line
// cannot be formatted at all: the count is this boundary's own fail-loud.
template <Kind kind>
using Lines =
    std::array<std::string, source::wl_stable::scheme::getLinesPerFile(kind)>;

template <Kind kind> struct SourceFile {
  static_assert(source::wl_stable::scheme::getLinesPerFile(kind) > 0,
                "a source with 0 lines is not a valid source");

  std::string path;
  Lines<kind> lines;
};

struct Sources {
  SourceFile<Kind::dictline> dictline;
  SourceFile<Kind::inflects> inflects;
  SourceFile<Kind::uniques> uniques;
  SourceFile<Kind::addons> addons;
};

using DictlineSource = decltype(Sources::dictline);
using InflectsSource = decltype(Sources::inflects);
using UniquesSource = decltype(Sources::uniques);
using AddonsSource = decltype(Sources::addons);

} // namespace formatted
