#include "line.hpp"

#include <string_view>
#include <utility>
#include <variant>

#include "fix.hpp"
#include "gen/tokenize/wl_stable/common/pool.hpp"
#include "grammar.hpp"
#include "meaning.hpp"

namespace wl_stable::tokenize::internal::addons {

namespace scheme = source::wl_stable::addons::scheme;

namespace {

tokenized::Addon placed(tokenized::Addon addon, text::Pool& pool) {
  std::visit([&](auto& a) { a.fix = place(a.fix, pool); }, addon);
  return addon;
}

} // namespace

void line(const Entry& entry, std::size_t index, std::string_view where,
          text::StringPools& pools, tokenized::Addons& into) {
  const Fix fix =
      extractFix(entry[std::to_underlying(scheme::Line::fix)], where);
  into.addon[index] = placed(
      extractGrammar(fix, entry[std::to_underlying(scheme::Line::partEntry)],
                     where),
      pools.addonsFixes);
  into.meaning[index] = place(
      extractMeaning(entry[std::to_underlying(scheme::Line::meaning)], where),
      pools.addonsMeanings);
}

} // namespace wl_stable::tokenize::internal::addons
