#include "senses.hpp"

#include <algorithm>
#include <string>

#include "error/error.hpp"
#include "source/wl_stable/uniques_scheme.hpp"

namespace wl_stable::tokenize::internal::uniques {

namespace scheme = source::wl_stable::uniques::scheme;

std::string_view extractSenses(std::string_view meaning,
                               std::string_view where) {
  if (meaning.size() < scheme::kSensesMinLength)
    error::fatal(std::string{where} + ": entry has no meaning");
  const auto low = std::ranges::find_if(
      meaning, [](char c) {
        const auto byte = static_cast<unsigned char>(c);
        return byte < static_cast<unsigned char>(scheme::kSensesLowestByte) ||
               byte > static_cast<unsigned char>(scheme::kSensesHighestByte);
      });
  if (low != meaning.end())
    error::fatal(std::string{where} +
                 ": meaning holds a byte outside printable ASCII at offset " +
                 std::to_string(low - meaning.begin()));
  return meaning;
}

} // namespace wl_stable::tokenize::internal::uniques
