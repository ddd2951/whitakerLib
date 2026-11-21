#include "execution_steps/steps.hpp"

#include "search/relationship_schema.hpp"
#include "storage/by_stem.hpp"
#include "storage/perfect_suffixes.hpp"
#include "storage/rows.hpp"

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::collectPerfectSuffixes() {
  using storage::byStem;
  using storage::perfectSuffixes;
  using storage::rows;
  for (const auto& [address, ids] : byStem)
    if (address.second == 3)
      for (const auto id : ids)
        if (rel::unpackGrammar(rows[id].grammar).analysis.part == latin::Part::verb)
          perfectSuffixes.insert(rows[id].suffix);
}
