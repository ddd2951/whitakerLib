#include "meaning.hpp"

#include <algorithm>
#include <cstddef>
#include <string>

#include "error/error.hpp"
#include "source/wl_stable/addons_scheme.hpp"

namespace wl_stable::tokenize::internal::addons {

namespace scheme = source::wl_stable::addons::scheme;

std::string_view extractMeaning(std::string_view line, std::string_view where) {
  const std::size_t end = line.find_last_not_of(scheme::kTrailingPadding);
  const std::string_view meaning = end == std::string_view::npos
                                       ? std::string_view{}
                                       : line.substr(0, end + 1);
  if (meaning.size() < scheme::kMeaningMinLength)
    error::fatal(std::string{where} + ": entry has no meaning");
  const auto low = std::ranges::find_if(
      meaning, [](char c) {
        const auto byte = static_cast<unsigned char>(c);
        return byte < static_cast<unsigned char>(scheme::kMeaningLowestByte) ||
               byte > static_cast<unsigned char>(scheme::kMeaningHighestByte);
      });
  if (low != meaning.end())
    error::fatal(std::string{where} +
                 ": meaning holds a byte outside printable ASCII at offset " +
                 std::to_string(low - meaning.begin()));
  return meaning;
}

} // namespace wl_stable::tokenize::internal::addons
