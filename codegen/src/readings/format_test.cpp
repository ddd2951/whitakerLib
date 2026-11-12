#include "readings/format.hpp"

namespace {
constexpr char kWritten[] = {
#embed "../../../include/latin_generated.hpp"
};
static_assert(std::string_view{kWritten, sizeof kWritten} == reading_format::kText,
              "include/latin_generated.hpp is stale: build write_reading_format");
} // namespace
