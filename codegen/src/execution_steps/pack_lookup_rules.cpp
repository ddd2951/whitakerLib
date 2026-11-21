#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "error/error.hpp"
#include "storage/result.hpp"
#include "storage/rows.hpp"
#include "storage/order.hpp"
#include "storage/array_order.hpp"
#include "storage/lookup_metadata.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <tuple>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::packLookupRules() {
  using storage::arrayOrder;
  using storage::order;
  using storage::out;
  using storage::result;
  using storage::rows;
  namespace te = whitaker::two_ended;
  unsigned maximumSource = 0;
  std::size_t maximumEnding = 0;
  std::map<std::string, unsigned> fullEndings{};
  for (unsigned id = 0; id < rows.size(); ++id) {
    const auto& [sourceRow, suffix, ending, grammar, sweep, key, age, freq, correction] = rows[id];
    maximumSource = std::max(maximumSource, sourceRow);
    maximumEnding = std::max(maximumEnding, ending.size());
    const auto [it, fresh] = fullEndings.try_emplace(ending, static_cast<unsigned>(fullEndings.size()));
    result.ruleSuffixes.push_back(suffix);
    const auto g = te::printed(rel::unpackGrammar(grammar).analysis);
    latin::packed::Reading r{};
    r.part = g.part;
    if (!latin::packed::packGrammar(g, r.g) || latin::packed::grammarOf(r) != g)
      error::fatal("two-ended: grammar does not fit");
    assert(key <= 4);
    result.rules.push_back({{latin::packed::head(r), {r.g[0], r.g[1], r.g[2]}},
                            order[id],
                            arrayOrder[id],
                            static_cast<std::uint32_t>(grammar),
                            static_cast<std::uint16_t>(sourceRow),
                            static_cast<std::uint16_t>(it->second),
                            static_cast<std::uint8_t>(key),
                            static_cast<std::uint8_t>(ending.size()),
                            latin::Age{static_cast<std::uint8_t>(age)},
                            latin::Frequency{static_cast<std::uint8_t>(freq)},
                            static_cast<std::int16_t>(correction)});
  }
  out << "static_assert(" << maximumSource << " <= 65535);\nstatic_assert(" << maximumEnding << " <= 255);\n";
}
