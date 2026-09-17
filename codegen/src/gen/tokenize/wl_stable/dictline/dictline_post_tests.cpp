#include "dictline_post_tests.hpp"

#include <array>
#include <cstddef>
#include <print>
#include <string>
#include <string_view>
#include <variant>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/pool.hpp"
#include "source/wl_stable/dictline_scheme.hpp"

namespace wl_stable::tokenize::internal::dictline::post_tests {

namespace scheme = source::wl_stable::dictline::scheme;

void verifyStemPool(const tokenized::Dictline& dictline,
                    const text::Pool& stems) {
  const auto verify = [&](const auto& column, std::string_view name) {
    for (std::size_t index = 0; index < column.size(); ++index) {
      const tokenized::Stem stem = column[index];
      if (tokenized::stemState(stem) != tokenized::StemState::text)
        continue;
      if (!inside(stem, stems))
        error::fatal("dictline record " + std::to_string(index + 1) + ": " +
                     std::string{name} + " views outside the stem pool");
    }
  };
  verify(dictline.stem1, "stem1");
  verify(dictline.stem2, "stem2");
  verify(dictline.stem3, "stem3");
  verify(dictline.stem4, "stem4");
}

void verifySensesPool(const tokenized::Dictline& dictline,
                      const text::Pool& senses) {
  for (std::size_t index = 0; index < dictline.senses.size(); ++index)
    if (!inside(dictline.senses[index], senses))
      error::fatal("dictline record " + std::to_string(index + 1) +
                   ": senses view outside the senses pool");
}

void reportDictlineLedger(const tokenized::Dictline& dictline,
                          const text::StringPools& pools) {
  std::println("tokenized dictline: {} records, stem pool {} bytes, senses "
               "pool {} bytes",
               dictline.grammar.size(), pools.dictlineStems.size(),
               pools.dictlineSenses.size());

  const auto stems = [](const auto& column, std::string_view name) {
    std::size_t text = 0, absent = 0, empty = 0;
    for (const tokenized::Stem stem : column)
      switch (tokenized::stemState(stem)) {
      case tokenized::StemState::text:
        ++text;
        break;
      case tokenized::StemState::absent:
        ++absent;
        break;
      case tokenized::StemState::empty:
        ++empty;
        break;
      }
    std::println("  {}: text {} zzz {} blank {}", name, text, absent, empty);
  };
  stems(dictline.stem1, "stem1");
  stems(dictline.stem2, "stem2");
  stems(dictline.stem3, "stem3");
  stems(dictline.stem4, "stem4");

  static_assert(scheme::kParts.size() ==
                std::variant_size_v<tokenized::Grammar>);
  std::array<std::size_t, scheme::kParts.size()> parts{};
  for (const tokenized::Grammar& grammar : dictline.grammar)
    ++parts[grammar.index()];
  std::string line = "  parts:";
  for (std::size_t i = 0; i < scheme::kParts.size(); ++i)
    line += " " + std::string{scheme::kParts[i]} + " " +
            std::to_string(parts[i]);
  std::println("{}", line);
}

} // namespace wl_stable::tokenize::internal::dictline::post_tests
