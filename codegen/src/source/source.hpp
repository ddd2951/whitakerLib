#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "shared/semantic/value.hpp"
#include "source/schemes.hpp"

namespace source {

struct DictlineIndexTag;
struct InflectsIndexTag;
struct UniquesIndexTag;
struct AddonsIndexTag;

using DictlineIndex = semantic::Value<DictlineIndexTag, std::size_t>;
using InflectsIndex = semantic::Value<InflectsIndexTag, std::size_t>;
using UniquesIndex = semantic::Value<UniquesIndexTag, std::size_t>;
using AddonsIndex = semantic::Value<AddonsIndexTag, std::size_t>;

using UniquesLines =
    std::array<std::string_view, scheme::uniques::kLinesPerEntry>;
using AddonsLines =
    std::array<std::string_view, scheme::addons::kLinesPerEntry>;

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
