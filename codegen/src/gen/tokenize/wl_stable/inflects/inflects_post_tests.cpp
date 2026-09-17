#include "inflects_post_tests.hpp"

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
#include "source/wl_stable/inflects_scheme.hpp"
#include "types/types.hpp"

namespace wl_stable::tokenize::internal::inflects::post_tests {

namespace scheme = source::wl_stable::inflects::scheme;

void verifyEndingPool(const tokenized::Inflects& inflects,
                      const text::Pool& endings) {
  for (std::size_t index = 0; index < inflects.ending.size(); ++index) {
    const std::string_view ending = inflects.ending[index];
    if (!ending.empty() && !inside(ending, endings))
      error::fatal("inflects record " + std::to_string(index + 1) +
                   ": ending views outside the ending pool");
  }
}

void verifyCharacterCounts(const tokenized::Inflects& inflects) {
  for (std::size_t index = 0; index < inflects.ending.size(); ++index) {
    const std::size_t declared = inflects.characterCount[index].value;
    const std::size_t held = inflects.ending[index].size();
    if (declared > held)
      error::fatal("inflects record " + std::to_string(index + 1) +
                   ": declares " + std::to_string(declared) +
                   " characters, ending holds " + std::to_string(held));
  }
}

void reportInflectsLedger(const tokenized::Inflects& inflects,
                          const text::StringPools& pools) {
  std::println("tokenized inflects: {} records, ending pool {} bytes",
               inflects.grammar.size(), pools.inflectsEndings.size());

  static_assert(scheme::kParts.size() ==
                std::variant_size_v<tokenized::Inflection>);
  std::array<std::size_t, scheme::kParts.size()> parts{};
  for (const tokenized::Inflection& grammar : inflects.grammar)
    ++parts[grammar.index()];
  std::string line = "  parts:";
  for (std::size_t i = 0; i < scheme::kParts.size(); ++i)
    line += " " + std::string{scheme::kParts[i]} + " " +
            std::to_string(parts[i]);
  std::println("{}", line);

  std::size_t present = 0, absent = 0, disagree = 0;
  std::map<std::size_t, std::size_t> count, key;
  std::map<TypeAge, std::size_t> age;
  std::map<TypeFrequency, std::size_t> frequency;
  for (std::size_t i = 0; i < inflects.grammar.size(); ++i) {
    const std::string_view ending = inflects.ending[i];
    ending.empty() ? ++absent : ++present;
    if (!ending.empty() && ending.size() != inflects.characterCount[i].value)
      ++disagree;
    ++count[inflects.characterCount[i].value];
    ++key[inflects.stemKey[i].value];
    ++age[inflects.age[i]];
    ++frequency[inflects.frequency[i]];
  }
  std::println("  ending: present {} absent {}; count != length on {} records",
               present, absent, disagree);
  std::println("  character count:{}", counts(count));
  std::println("  stem key:{}", counts(key));
  std::println("  age:{}", counts(age));
  std::println("  frequency:{}", counts(frequency));
}

} // namespace wl_stable::tokenize::internal::inflects::post_tests
