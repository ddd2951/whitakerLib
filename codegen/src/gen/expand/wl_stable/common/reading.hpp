#pragma once

#include <string_view>

#include "gen/expand/wl_stable/common/form.hpp"
#include "gen/expand/wl_stable/common/synthetics.hpp"
#include "types/domain.hpp"
#include "types/tokenized_sources.hpp"
#include "types/types.hpp"

namespace wl_stable::expand {

struct Reading {
  std::string_view stem;
  domain::StemIndex key;
  tokenized::Grammar entry;
  tokenized::Inflection inflection;
  TypeAge entryAge;
  TypeArea area;
  TypeGeography geography;
  TypeFrequency entryFrequency;
  TypeSource source;
  TypeAge inflectionAge;
  TypeFrequency inflectionFrequency;
  std::string_view senses;
  bool unique;
};

[[nodiscard]] Reading readingOf(const tokenized::Sources& sources,
                                const Synthetics& synthetics,
                                const Origin& origin);

} // namespace wl_stable::expand
