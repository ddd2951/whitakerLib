#include "inflects.hpp"

#include <cstddef>
#include <string>

#include "gen/tokenize/wl_stable/common/frame.hpp"
#include "line.hpp"
#include "source/wl_stable/inflects_scheme.hpp"

namespace wl_stable::tokenize {

namespace scheme = source::wl_stable::inflects::scheme;

void tokenizeInflects(const formatted::InflectsSource& source,
                      text::StringPools& pools, tokenized::Inflects& into) {
  std::size_t bytes = 0;
  for (const std::string& text : source.lines)
    bytes += text.size();
  pools.inflectsEndings.reserve(bytes);

  internal::frame(
      source, scheme::kCommentMarker, scheme::kWhitespace,
      [&](std::size_t index, std::size_t number, std::string_view text) {
        internal::inflects::line(text, index,
                                 source.path + ":" + std::to_string(number),
                                 pools, into);
      });
}

} // namespace wl_stable::tokenize
