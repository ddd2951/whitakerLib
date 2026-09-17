#include "uniques.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include "line.hpp"
#include "source/wl_stable/uniques_scheme.hpp"

namespace wl_stable::tokenize {

namespace scheme = source::wl_stable::uniques::scheme;

void tokenizeUniques(const formatted::UniquesSource& source,
                     text::StringPools& pools, tokenized::Uniques& into) {
  std::size_t bytes = 0;
  for (const std::string& text : source.lines)
    bytes += text.size();
  pools.uniquesForms.reserve(bytes);
  pools.uniquesSenses.reserve(bytes);

  for (std::size_t index = 0; index < scheme::kEntriesPerFile; ++index) {
    const std::size_t first = index * scheme::kLinesPerEntry;
    internal::uniques::line(
        source.lines[first + std::to_underlying(scheme::Line::word)],
        source.lines[first + std::to_underlying(scheme::Line::attributes)],
        source.lines[first + std::to_underlying(scheme::Line::meaning)], index,
        source.path + ":" + std::to_string(first + 1), pools, into);
  }
}

} // namespace wl_stable::tokenize
