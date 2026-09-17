#include "part.hpp"

#include <string>
#include <string_view>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/unpadded.hpp"
#include "util/reflect_util.hpp"

namespace wl_stable::tokenize::internal::dictline {

namespace scheme = source::wl_stable::dictline::scheme;

PosTokenTypes extractPart(std::span<const char, scheme::kPartWidth> slice,
                          std::string_view where) {
  const std::string_view name = unpadded(slice, scheme::kFieldSeparator);
  for (const std::string_view part : scheme::kParts)
    if (part == name)
      return *util::trySvToEnum<PosTokenTypes>(name);
  error::fatal(std::string{where} + ": not a DICTLINE part: '" +
               std::string{name} + "'");
}

} // namespace wl_stable::tokenize::internal::dictline
