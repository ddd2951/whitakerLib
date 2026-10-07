#pragma once

#include <string_view>

#include "expand/common/form.hpp"
#include "latin.hpp"
#include "types/grammar.hpp"

namespace expand {

struct Reading {
  std::string_view stem;
  latin::StemKey key;
  latin::Entry entry;
  latin::Inflection inflection;
  latin::Age entryAge;
  latin::Frequency entryFrequency;
  latin::Age inflectionAge;
  latin::Frequency inflectionFrequency;
  std::string_view senses;
  bool unique;
};

[[nodiscard]] Reading readingOf(const Origin& origin);

} // namespace expand
