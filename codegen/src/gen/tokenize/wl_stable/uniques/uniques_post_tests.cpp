#include "uniques_post_tests.hpp"

#include <array>
#include <cstddef>
#include <map>
#include <print>
#include <string>
#include <string_view>
#include <variant>

#include "error/error.hpp"
#include "gen/tokenize/wl_stable/common/pool.hpp"
#include "gen/tokenize/wl_stable/common/report.hpp"
#include "source/wl_stable/uniques_scheme.hpp"
#include "types/types.hpp"

namespace wl_stable::tokenize::internal::uniques::post_tests {

namespace scheme = source::wl_stable::uniques::scheme;

void verifyPools(const tokenized::Uniques& uniques,
                 const text::StringPools& pools) {
  for (std::size_t index = 0; index < uniques.form.size(); ++index) {
    if (!inside(uniques.form[index], pools.uniquesForms))
      error::fatal("uniques entry " + std::to_string(index + 1) +
                   ": form views outside the form pool");
    if (!inside(uniques.senses[index], pools.uniquesSenses))
      error::fatal("uniques entry " + std::to_string(index + 1) +
                   ": senses view outside the senses pool");
  }
}

void reportUniquesLedger(const tokenized::Uniques& uniques,
                         const text::StringPools& pools) {
  std::println(
      "tokenized uniques: {} entries, form pool {} bytes, senses pool {} bytes",
      uniques.form.size(), pools.uniquesForms.size(),
      pools.uniquesSenses.size());

  static_assert(scheme::kParts.size() ==
                std::variant_size_v<tokenized::UniqueGrammar>);
  std::array<std::size_t, scheme::kParts.size()> parts{};
  std::size_t lowercaseKinds = 0;
  for (const tokenized::UniqueGrammar& grammar : uniques.grammar) {
    ++parts[grammar.index()];
    if (const auto* noun = std::get_if<tokenized::unique::Noun>(&grammar))
      if (static_cast<char>(noun->nounKind) >= 'a')
        ++lowercaseKinds;
  }
  std::string line = "  parts:";
  for (std::size_t i = 0; i < scheme::kParts.size(); ++i)
    line += " " + std::string{scheme::kParts[i]} + " " +
            std::to_string(parts[i]);
  std::println("{}; lowercase noun kinds {}", line, lowercaseKinds);

  std::map<TypeAge, std::size_t> age;
  std::map<TypeArea, std::size_t> area;
  std::map<TypeGeography, std::size_t> geography;
  std::map<TypeFrequency, std::size_t> frequency;
  std::map<TypeSource, std::size_t> source;
  for (std::size_t i = 0; i < uniques.form.size(); ++i) {
    ++age[uniques.age[i]];
    ++area[uniques.area[i]];
    ++geography[uniques.geography[i]];
    ++frequency[uniques.frequency[i]];
    ++source[uniques.source[i]];
  }
  std::println("  age:{}", counts(age));
  std::println("  area:{}", counts(area));
  std::println("  geography:{}", counts(geography));
  std::println("  frequency:{}", counts(frequency));
  std::println("  source:{}", counts(source));
}

} // namespace wl_stable::tokenize::internal::uniques::post_tests
