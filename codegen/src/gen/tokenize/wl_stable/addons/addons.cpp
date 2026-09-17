#include "addons.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#include "gen/tokenize/wl_stable/common/frame.hpp"
#include "line.hpp"
#include "source/wl_stable/addons_scheme.hpp"

namespace wl_stable::tokenize {

namespace scheme = source::wl_stable::addons::scheme;

void tokenizeAddons(const formatted::AddonsSource& source,
                    text::StringPools& pools, tokenized::Addons& into) {
  std::size_t bytes = 0;
  for (const std::string& text : source.lines)
    bytes += text.size();
  pools.addonsFixes.reserve(bytes);
  pools.addonsMeanings.reserve(bytes);

  // Comment lines fall between entries, so every third record line closes one.
  std::array<std::string_view, scheme::kLinesPerEntry> entry{};
  std::size_t firstLine = 0;
  internal::frame(
      source, scheme::kCommentMarker, scheme::kWhitespace,
      [&](std::size_t index, std::size_t number, std::string_view text) {
        const std::size_t at = index % scheme::kLinesPerEntry;
        if (at == 0)
          firstLine = number;
        entry[at] = text;
        if (at + 1 == scheme::kLinesPerEntry)
          internal::addons::line(entry, index / scheme::kLinesPerEntry,
                                 source.path + ":" + std::to_string(firstLine),
                                 pools, into);
      });
}

} // namespace wl_stable::tokenize
