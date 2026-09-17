#pragma once
#include <string_view>

// NOTE: Generator choices that are neither corpus constraints nor derived
// values.

namespace main_config {

#ifndef GEN_DATA_DIR
#define GEN_DATA_DIR "../data"
#endif

constexpr std::string_view kDataDir{GEN_DATA_DIR};

// NOTE: Generated. Do not touch.
#ifndef GEN_VERSION
#error "GEN_VERSION is not defined; the target must link gen_config."
#endif

constexpr std::string_view kGeneratorBanner{"whitaker-gen " GEN_VERSION};

// INFO: The original WORDS files the readers construct from, vendored under
//  data/source/ with the hashes in its MANIFEST.sha256.
constexpr std::string_view kSourceDictlinePath{GEN_DATA_DIR
                                               "/source/DICTLINE.GEN"};
constexpr std::string_view kSourceInflectsPath{GEN_DATA_DIR
                                               "/source/INFLECTS.LAT"};
constexpr std::string_view kSourceUniquesPath{GEN_DATA_DIR
                                              "/source/UNIQUES.LAT"};
constexpr std::string_view kSourceAddonsPath{GEN_DATA_DIR "/source/ADDONS.LAT"};
constexpr std::string_view kOutputPath{GEN_DATA_DIR "/whitaker.dat"};

} // namespace main_config
