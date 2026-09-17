#include "part.hpp"

#include <string>

#include "error/error.hpp"
#include "source/wl_stable/inflects_scheme.hpp"
#include "util/reflect_util.hpp"

namespace wl_stable::tokenize::internal::inflects {

namespace scheme = source::wl_stable::inflects::scheme;

PosTokenTypes extractPart(std::string_view token, std::string_view where) {
  for (const std::string_view part : scheme::kParts)
    if (part == token)
      return *util::trySvToEnum<PosTokenTypes>(token);
  error::fatal(std::string{where} + ": not an INFLECTS part: '" +
               std::string{token} + "'");
}

} // namespace wl_stable::tokenize::internal::inflects
