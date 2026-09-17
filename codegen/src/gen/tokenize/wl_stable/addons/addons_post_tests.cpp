#include "addons_post_tests.hpp"

#include <cstddef>
#include <map>
#include <print>
#include <string>
#include <string_view>
#include <variant>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/pool.hpp"
#include "gen/tokenize/wl_stable/common/report.hpp"
#include "types/types.hpp"

namespace wl_stable::tokenize::internal::addons::post_tests {

namespace {

using namespace tokenized;

std::string_view fixOf(const Addon& addon) {
  return std::visit([](const auto& a) { return a.fix; }, addon);
}

} // namespace

void verifyPools(const Addons& addons, const text::StringPools& pools) {
  for (std::size_t index = 0; index < addons.addon.size(); ++index) {
    if (!inside(fixOf(addons.addon[index]), pools.addonsFixes))
      error::fatal("addons entry " + std::to_string(index + 1) +
                   ": fix views outside the fix pool");
    if (!inside(addons.meaning[index], pools.addonsMeanings))
      error::fatal("addons entry " + std::to_string(index + 1) +
                   ": meaning views outside the meaning pool");
  }
}

void reportAddonsLedger(const Addons& addons, const text::StringPools& pools) {
  std::println(
      "tokenized addons: {} entries, fix pool {} bytes, meaning pool {} bytes",
      addons.addon.size(), pools.addonsFixes.size(),
      pools.addonsMeanings.size());

  std::size_t prefixes = 0, suffixes = 0, tackons = 0, connects = 0;
  std::map<PosTokenTypes, std::size_t> prefixTo, suffixFrom, suffixTarget,
      tackonTarget;
  std::map<std::size_t, std::size_t> fromKey, toKey, fixLength;
  for (const Addon& addon : addons.addon) {
    ++fixLength[fixOf(addon).size()];
    if (const auto* p = std::get_if<addon::Prefix>(&addon)) {
      ++prefixes;
      if (p->connect)
        ++connects;
      ++prefixTo[p->to];
    } else if (const auto* s = std::get_if<addon::Suffix>(&addon)) {
      ++suffixes;
      if (s->connect)
        ++connects;
      ++suffixFrom[s->from];
      ++suffixTarget[s->target];
      ++fromKey[s->fromKey.value];
      ++toKey[s->toKey.value];
    } else if (const auto* t = std::get_if<addon::Tackon>(&addon)) {
      ++tackons;
      ++tackonTarget[t->target];
    }
  }
  std::println("  kinds: PREFIX {} SUFFIX {} TACKON {}; connect present {}",
               prefixes, suffixes, tackons, connects);
  std::println("  PREFIX to:{}", counts(prefixTo));
  std::println("  SUFFIX from:{}", counts(suffixFrom));
  std::println("  SUFFIX target:{}", counts(suffixTarget));
  std::println("  SUFFIX fromKey:{}  toKey:{}", counts(fromKey), counts(toKey));
  std::println("  TACKON target:{}", counts(tackonTarget));
  std::println("  fix length:{}", counts(fixLength));
}

} // namespace wl_stable::tokenize::internal::addons::post_tests
