#pragma once

#include <string>
#include <string_view>

namespace text {

// NOTE: Views into a pool are taken as it is filled. A writer reserves the
// pool before its first append, or every earlier view dangles.
using Pool = std::string;

struct StringPools {
  // An absent stem is a view of this, outside every pool.
  static constexpr std::string_view kAbsentStem{"zzz"};

  Pool dictlineStems;
  Pool dictlineSenses;
  Pool inflectsEndings;
  Pool uniquesForms;
  Pool uniquesSenses;
  Pool addonsFixes;
  Pool addonsMeanings;
};

} // namespace text
