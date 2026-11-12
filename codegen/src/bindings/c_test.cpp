// INFO: gen does not build while bindings/c/whitaker.h, whitaker_c.inc or inspector/api_view.inc is not what
//  the tags give today.
#include "bindings/schemes/c.hpp"
#include "bindings/web.hpp"

#include <string_view>

namespace {

constexpr char kHeader[] = {
#embed "../../../bindings/c/whitaker.h"
};
constexpr char kWrappers[] = {
#embed "../../../bindings/c/whitaker_c.inc"
};
constexpr char kApiView[] = {
#embed "../../../inspector/api_view.inc"
};

static_assert(std::string_view{kHeader, sizeof kHeader} == bindings::c::kHeader,
              "bindings/c/whitaker.h is stale: cmake --build build-codegen --target write_bindings");
static_assert(std::string_view{kWrappers, sizeof kWrappers} == bindings::c::kWrappers,
              "bindings/c/whitaker_c.inc is stale: cmake --build build-codegen --target write_bindings");
static_assert(std::string_view{kApiView, sizeof kApiView} == bindings::web::kText,
              "inspector/api_view.inc is stale: cmake --build build-codegen --target write_bindings");

} // namespace
