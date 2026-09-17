#include "pre_tests.hpp"

#include <print>

#include "source/wl_stable/addons_scheme.hpp"
#include "source/wl_stable/dictline_scheme.hpp"
#include "source/wl_stable/inflects_scheme.hpp"
#include "source/wl_stable/uniques_scheme.hpp"

namespace wl_stable::tokenize::internal::pre_tests {

static_assert(partsAreNamed(source::wl_stable::dictline::scheme::kParts),
              "dictline scheme: kParts holds a token that is not a part");
static_assert(partsAreNamed(source::wl_stable::inflects::scheme::kParts),
              "inflects scheme: kParts holds a token that is not a part");
static_assert(partsAreNamed(source::wl_stable::uniques::scheme::kParts),
              "uniques scheme: kParts holds a token that is not a part");
static_assert(partsAreNamed(source::wl_stable::addons::scheme::kParts),
              "addons scheme: kParts holds a token that is not a part");

void verifySchemeParts() {
  std::println("tokenize pre: every scheme's kParts names a part of speech "
               "(proved at compile time)");
}

} // namespace wl_stable::tokenize::internal::pre_tests
