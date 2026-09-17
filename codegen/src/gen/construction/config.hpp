#pragma once

#include <main_config.hpp>

#include "source/wl_stable_schemes.hpp"

#include <string_view>

namespace construction::config {

constexpr std::string_view kDictlinePath{::main_config::kSourceDictlinePath};
constexpr std::string_view kInflectsPath{::main_config::kSourceInflectsPath};
constexpr std::string_view kUniquesPath{::main_config::kSourceUniquesPath};
constexpr std::string_view kAddonsPath{::main_config::kSourceAddonsPath};

// NOTE: No two paths may name one file. Added after it happened.
static_assert(kDictlinePath != kInflectsPath,
              "DICTLINE.GEN and INFLECTS.LAT are the same path");
static_assert(kDictlinePath != kUniquesPath,
              "DICTLINE.GEN and UNIQUES.LAT are the same path");
static_assert(kDictlinePath != kAddonsPath,
              "DICTLINE.GEN and ADDONS.LAT are the same path");
static_assert(kInflectsPath != kUniquesPath,
              "INFLECTS.LAT and UNIQUES.LAT are the same path");
static_assert(kInflectsPath != kAddonsPath,
              "INFLECTS.LAT and ADDONS.LAT are the same path");
static_assert(kUniquesPath != kAddonsPath,
              "UNIQUES.LAT and ADDONS.LAT are the same path");

consteval std::string_view
getPath(::source::wl_stable::scheme::SourceFileKind kind) {
  using ::source::wl_stable::scheme::SourceFileKind;
  switch (kind) {
  case SourceFileKind::dictline:
    return kDictlinePath;
  case SourceFileKind::inflects:
    return kInflectsPath;
  case SourceFileKind::uniques:
    return kUniquesPath;
  case SourceFileKind::addons:
    return kAddonsPath;
  }
}

} // namespace construction::config
