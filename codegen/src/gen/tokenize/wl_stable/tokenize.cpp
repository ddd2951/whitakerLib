#include "tokenize.hpp"

#include <print>

#include "gen/tokenize/wl_stable/addons/addons.hpp"
#include "gen/tokenize/wl_stable/addons/addons_post_tests.hpp"
#include "gen/tokenize/wl_stable/common/pre_tests.hpp"
#include "gen/tokenize/wl_stable/dictline/dictline.hpp"
#include "gen/tokenize/wl_stable/dictline/dictline_post_tests.hpp"
#include "gen/tokenize/wl_stable/inflects/inflects.hpp"
#include "gen/tokenize/wl_stable/inflects/inflects_post_tests.hpp"
#include "gen/tokenize/wl_stable/uniques/uniques.hpp"
#include "gen/tokenize/wl_stable/uniques/uniques_post_tests.hpp"

void wl_stable::tokenize::pre() {
  internal::pre_tests::verifySchemeParts();
  std::println("wl_stable tokenize pre has run");
}

void wl_stable::tokenize::run(const formatted::Sources& formatted,
                              text::StringPools& pools,
                              tokenized::Sources& into) {
  tokenizeDictline(formatted.dictline, pools, into.dictline);
  tokenizeInflects(formatted.inflects, pools, into.inflects);
  tokenizeUniques(formatted.uniques, pools, into.uniques);
  tokenizeAddons(formatted.addons, pools, into.addons);
  std::println("wl_stable tokenize has run");
}

void wl_stable::tokenize::post(const tokenized::Sources& tokenized,
                               const text::StringPools& pools) {
  namespace dictlineTests = internal::dictline::post_tests;
  dictlineTests::reportDictlineLedger(tokenized.dictline, pools);
  dictlineTests::verifyStemPool(tokenized.dictline, pools.dictlineStems);
  dictlineTests::verifySensesPool(tokenized.dictline, pools.dictlineSenses);
  namespace inflectsTests = internal::inflects::post_tests;
  inflectsTests::reportInflectsLedger(tokenized.inflects, pools);
  inflectsTests::verifyEndingPool(tokenized.inflects, pools.inflectsEndings);
  inflectsTests::verifyCharacterCounts(tokenized.inflects);
  namespace uniquesTests = internal::uniques::post_tests;
  uniquesTests::reportUniquesLedger(tokenized.uniques, pools);
  uniquesTests::verifyPools(tokenized.uniques, pools);
  namespace addonsTests = internal::addons::post_tests;
  addonsTests::reportAddonsLedger(tokenized.addons, pools);
  addonsTests::verifyPools(tokenized.addons, pools);
  std::println("wl_stable tokenize post has run");
}
