#include "dictline.hpp"

#include <cstddef>
#include <string>

#include "line.hpp"

namespace wl_stable::tokenize {

void tokenizeDictline(const formatted::DictlineSource& source,
                      text::StringPools& pools, tokenized::Dictline& into) {
  std::size_t bytes = 0;
  for (const std::string& text : source.lines)
    bytes += text.size();
  pools.dictlineStems.reserve(bytes);
  pools.dictlineSenses.reserve(bytes);

  for (std::size_t index = 0; index < source.lines.size(); ++index)
    internal::dictline::line(source.lines[index], index,
                             source.path + ":" + std::to_string(index + 1),
                             pools, into);
}

} // namespace wl_stable::tokenize
