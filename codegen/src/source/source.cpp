#include "source.hpp"

#include "error/error.hpp"
#include "execution_steps/steps.hpp"
#include "source/schemes.hpp"
#include "storage/lines.hpp"
#include "storage/records.hpp"

#include <array>
#include <cstddef>
#include <meta>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace scheme = source::scheme;

using storage::lines;

using storage::records;

template <std::meta::info file> std::string_view recordLine(const std::size_t entry, const std::size_t part) {
  return lines<file>[records<file>[entry * [:file:] ::kLinesPerEntry + part]];
}

template <std::meta::info file> std::size_t firstLineNumber(const std::size_t entry) {
  return records<file>[entry * [:file:] ::kLinesPerEntry] + 1;
}

} // namespace

std::string_view source::dictline(const DictlineIndex entry) { return recordLine<^^scheme::dictline>(entry.value, 0); }
std::string_view source::inflects(const InflectsIndex entry) { return recordLine<^^scheme::inflects>(entry.value, 0); }
source::UniquesLines source::uniques(const UniquesIndex entry) {
  return {recordLine<^^scheme::uniques>(entry.value, 0), recordLine<^^scheme::uniques>(entry.value, 1),
          recordLine<^^scheme::uniques>(entry.value, 2)};
}
source::AddonsLines source::addons(const AddonsIndex entry) {
  return {recordLine<^^scheme::addons>(entry.value, 0), recordLine<^^scheme::addons>(entry.value, 1),
          recordLine<^^scheme::addons>(entry.value, 2)};
}

std::size_t source::lineNumber(const DictlineIndex entry) { return firstLineNumber<^^scheme::dictline>(entry.value); }
std::size_t source::lineNumber(const InflectsIndex entry) { return firstLineNumber<^^scheme::inflects>(entry.value); }
std::size_t source::lineNumber(const UniquesIndex entry) { return firstLineNumber<^^scheme::uniques>(entry.value); }
std::size_t source::lineNumber(const AddonsIndex entry) { return firstLineNumber<^^scheme::addons>(entry.value); }
