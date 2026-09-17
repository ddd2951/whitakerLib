#include "stem.hpp"

#include <string>
#include <string_view>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/unpadded.hpp"
#include "types/string_pools.hpp"

namespace wl_stable::tokenize::internal::dictline {

namespace scheme = source::wl_stable::dictline::scheme;

tokenized::Stem extractStem(std::span<const char, scheme::kStemWidth> slice,
                            std::string_view where) {
  const std::string_view unpad = unpadded(slice, scheme::kFieldSeparator);
  if (unpad.empty())
    return {};
  if (unpad == text::StringPools::kAbsentStem)
    return text::StringPools::kAbsentStem;
  if (unpad.contains(scheme::kFieldSeparator))
    error::fatal(std::string{where} + ": stem has a space inside its text");
  return unpad;
}

} // namespace wl_stable::tokenize::internal::dictline
