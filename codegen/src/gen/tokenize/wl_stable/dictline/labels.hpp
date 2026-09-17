#pragma once

#include <span>
#include <string_view>

#include "source/wl_stable/dictline_scheme.hpp"
#include "types/types.hpp"

namespace wl_stable::tokenize::internal::dictline {

struct Labels {
  TypeAge age;
  TypeArea area;
  TypeGeography geography;
  TypeFrequency frequency;
  TypeSource source;
};

Labels extractLabels(
    std::span<const char, source::wl_stable::dictline::scheme::kLabelsWidth>
        slice,
    std::string_view where);

} // namespace wl_stable::tokenize::internal::dictline
