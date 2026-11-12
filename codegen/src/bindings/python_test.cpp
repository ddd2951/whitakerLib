// INFO: gen does not build while bindings/python/whitaker_generated.inc is not what the tags give today.
#include "bindings/schemes/python.hpp"

#include <string_view>

namespace {

constexpr char kWritten[] = {
#embed "../../../bindings/python/whitaker_generated.inc"
};

static_assert(std::string_view{kWritten, sizeof kWritten} == bindings::python::kText,
              "bindings/python/whitaker_generated.inc is stale: cmake --build build-codegen --target write_bindings");

} // namespace
