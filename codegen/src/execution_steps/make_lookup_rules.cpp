#include "execution_steps/steps.hpp"

#include "helpers/adverb_of.hpp"
#include "helpers/rule.hpp"
#include "expand/common/join.hpp"
#include "storage/by_stem.hpp"
#include "storage/candidate_counts.hpp"
#include "storage/lengths.hpp"
#include "storage/mixed.hpp"
#include "storage/compression_duplicates.hpp"
#include <set>
#include <tuple>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::makeLookupRules() {
  using helpers::rule;
  using storage::byStem;
  using storage::candidateCounts;
  using storage::compressionDuplicates;
  using storage::lengths;
  using storage::mixed;
  std::set<std::tuple<std::string, unsigned, std::uint64_t>> specialQualities{};
  expand::internal::forEachLicensed([&](const expand::internal::Licensed& j) {
    const std::string spelling = emitter::normalize(std::string(j.stem) + std::string(j.suffix));
    if (!spelling.empty() && spelling.size() <= facts::kMaxWordCharacters)
      ++candidateCounts[spelling];
    const expand::Joined origin{j.entry, j.inflection, j.column};
    const auto r = expand::readingOf(origin);
    const auto g = emitter::analysisOf(r.entry, r.inflection, r.key);
    if (!spelling.empty() && spelling.size() <= facts::kMaxWordCharacters &&
        !(g.part == latin::Part::PRON && g.which.value == 1)) {
      const auto [it, fresh] = lengths.emplace(std::tuple{spelling, j.entry.value, rel::packGrammar(g)},
                                               static_cast<unsigned>(j.ending.ending.size()));
      if (!fresh && it->second != j.ending.ending.size())
        ++mixed;
    }
    // INFO: Ordinary rows are deduplicated by entry/quality in order(). Qu rows bypass that step;
    //  uniques have one row per entry. Prove those exceptions also have unique source qualities.
    if (!spelling.empty() && spelling.size() <= facts::kMaxWordCharacters && g.part == latin::Part::PRON &&
        g.which.value == 1 && !specialQualities.emplace(spelling, j.entry.value, rel::packGrammar(g)).second)
      ++compressionDuplicates;
    int correction = -1;
    if (const auto adverb = helpers::adverbOf(r))
      correction = static_cast<int>(rule(expand::Joined{j.entry, *adverb, j.column}, adverb->value, "",
                                         std::string(word::entry(*adverb).ending), -1));
    byStem[{j.entry.value, j.column.value}].push_back(
        rule(origin, j.inflection.value, emitter::normalize(j.suffix), std::string(j.ending.ending), correction));
  });
}
