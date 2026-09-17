#pragma once

#include <cstddef>
#include <string_view>

#include "grammar.hpp"
#include "types/domain.hpp"

namespace wl_stable::tokenize::internal::inflects {

struct Ending {
  domain::StemIndex stemKey;
  domain::CharacterCount characterCount;
  std::string_view text;
  std::size_t fields;
};

Ending extractEnding(const Fields& f, std::size_t at, std::string_view where);

} // namespace wl_stable::tokenize::internal::inflects
