#include "form.hpp"

#include <cstddef>
#include <string>

#include "error/error.hpp"
#include "source/wl_stable/uniques_scheme.hpp"

namespace wl_stable::tokenize::internal::uniques {

namespace scheme = source::wl_stable::uniques::scheme;

std::string_view extractForm(std::string_view word, std::string_view where) {
  const std::size_t end = word.find_last_not_of(scheme::kTrailingPadding);
  if (end == std::string_view::npos)
    error::fatal(std::string{where} + ": word line is blank");
  const std::string_view form = word.substr(0, end + 1);
  for (const char c : form)
    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
      error::fatal(std::string{where} + ": word '" + std::string{form} +
                   "' is not letters");
  return form;
}

} // namespace wl_stable::tokenize::internal::uniques
