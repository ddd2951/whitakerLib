#pragma once

#include <array>
#include <cstddef>
#include <meta>
#include <string_view>

#include "source/schemes.hpp"

namespace source {

template <std::meta::info file> struct Index {
  std::size_t value{};
  friend constexpr bool operator==(Index, Index) = default;
};

using DictlineIndex = Index<^^scheme::dictline>;
using InflectsIndex = Index<^^scheme::inflects>;
using UniquesIndex = Index<^^scheme::uniques>;
using AddonsIndex = Index<^^scheme::addons>;

using UniquesLines = std::array<std::string_view, scheme::uniques::kLinesPerEntry>;
using AddonsLines = std::array<std::string_view, scheme::addons::kLinesPerEntry>;

void init();

std::string_view dictline(DictlineIndex entry);
std::string_view inflects(InflectsIndex entry);
UniquesLines uniques(UniquesIndex entry);
AddonsLines addons(AddonsIndex entry);

std::size_t lineNumber(DictlineIndex entry);
std::size_t lineNumber(InflectsIndex entry);
std::size_t lineNumber(UniquesIndex entry);
std::size_t lineNumber(AddonsIndex entry);

} // namespace source
