#include "execution_steps/steps.hpp"

#include "latin.hpp"
#include "search/relationship_schema.hpp"
#include "storage/array_order.hpp"
#include "storage/order.hpp"
#include "storage/rows.hpp"
#include <algorithm>
#include <map>
#include <numeric>
#include <utility>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::rankLookupRules() {
  using storage::rows;
  // INFO: Within a dictionary entry, List_Sweep's order depends only on grammar, ending length and source ties.
  //  Its qu-pronoun comparison is asymmetric across declensions; rank each part/which family separately.
  std::map<std::pair<latin::Part, unsigned>, std::vector<unsigned>> families{};
  for (unsigned i = 0; i < rows.size(); ++i) {
    const auto g = rel::unpackGrammar(rows[i].sweep).analysis;
    families[{g.part, g.which.value}].push_back(i);
  }
  std::vector<unsigned> order(rows.size());
  unsigned rank = 0;
  for (auto& [family, ids] : families) {
    std::ranges::sort(ids, [&](const unsigned a, const unsigned b) {
      const auto x = rel::unpackGrammar(rows[a].sweep).analysis;
      const auto y = rel::unpackGrammar(rows[b].sweep).analysis;
      const auto nx = rows[a].ending.size(), ny = rows[b].ending.size();
      if (latin::listedBefore(x, nx, y, ny))
        return true;
      if (latin::listedBefore(y, ny, x, nx))
        return false;
      const auto sa = rows[a].sourceRow, sb = rows[b].sourceRow;
      if (sa != sb)
        return x.part == latin::Part::pronoun && x.which.value == 1 ? sa < sb : sa > sb;
      return a < b;
    });
    for (const auto id : ids)
      order[id] = rank++;
  }
  // INFO: Different parts of a verb's entry follow WORDS' part order, not enum ordinal order.
  for (unsigned i = 0; i < order.size(); ++i) {
    const auto g = rel::unpackGrammar(rows[i].sweep).analysis;
    order[i] += latin::pofsPos(g.part) * static_cast<unsigned>(rows.size());
  }
  std::vector<unsigned> rawIds(rows.size()), arrayOrder(rows.size());
  std::iota(rawIds.begin(), rawIds.end(), 0);
  std::ranges::sort(rawIds, [&](const unsigned a, const unsigned b) {
    const auto x = rel::unpackGrammar(rows[a].grammar).analysis;
    const auto y = rel::unpackGrammar(rows[b].grammar).analysis;
    if (latin::inflectionsLess(x, y))
      return true;
    if (latin::inflectionsLess(y, x))
      return false;
    return a < b;
  });
  for (unsigned i = 0; i < rawIds.size(); ++i)
    arrayOrder[rawIds[i]] = i;
  storage::order = std::move(order);
  storage::arrayOrder = std::move(arrayOrder);
}
