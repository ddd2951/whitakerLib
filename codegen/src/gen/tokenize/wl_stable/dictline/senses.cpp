#include "senses.hpp"

#include <algorithm>
#include <string>

#include "error/error.hpp"
#include "source/wl_stable/dictline_scheme.hpp"

namespace wl_stable::tokenize::internal::dictline {

namespace scheme = source::wl_stable::dictline::scheme;

std::string_view extractSenses(std::string_view rest, std::string_view where) {
  if (rest.size() < scheme::kSensesMinLength)
    error::fatal(std::string{where} + ": record has no senses");
  const auto low = std::ranges::find_if(
      rest, [](char c) {
        const auto byte = static_cast<unsigned char>(c);
        return byte < static_cast<unsigned char>(scheme::kSensesLowestByte) ||
               byte > static_cast<unsigned char>(scheme::kSensesHighestByte);
      });
  if (low != rest.end())
    error::fatal(std::string{where} +
                 ": senses hold a byte outside printable ASCII at offset " +
                 std::to_string(low - rest.begin()));
  return rest;
}

} // namespace wl_stable::tokenize::internal::dictline
