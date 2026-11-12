#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "search/two_ended_schema.hpp"
#include "storage/classes.hpp"
#include "storage/stems.hpp"
#include "storage/ending_ids.hpp"
#include "storage/rows.hpp"
#include "storage/order.hpp"
#include "storage/cells.hpp"
#include "storage/cell_of.hpp"
#include "storage/row_ids.hpp"
#include "storage/masks.hpp"
#include "storage/max_rows.hpp"
#include "storage/dropped_duplicates.hpp"
#include "storage/endings.hpp"
#include <algorithm>
#include <cassert>
#include <map>
#include <tuple>
#include <utility>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::buildLookupCells() {
  using storage::cells;
  using storage::classes;
  using storage::droppedDuplicates;
  using storage::endingIds;
  using storage::endings;
  using storage::masks;
  using storage::maxRows;
  using storage::order;
  using storage::rowIds;
  using storage::rows;
  using storage::stems;
  namespace te = whitaker::two_ended;
  std::vector<unsigned> cellOf(classes.size() * endings);
  // INFO: order() drops a row whose stem is plain and an earlier candidate has its entry and quality. Inside one cell
  //  that is settled here, so a class must be all plain stems or all unique and qu stems.
  std::vector<unsigned> classFlags(classes.size()), classPlain(classes.size());
  for (const auto& s : stems) {
    if (s.klass == rel::kNoLookupClass)
      continue;
    assert(s.klass < classes.size());
    classFlags[s.klass] |= s.flags;
    classPlain[s.klass] |= (s.flags & (te::kUnique | te::kQu)) == 0;
  }
  for (unsigned c = 0; c < classes.size(); ++c)
    if (classPlain[c] && (classFlags[c] & (te::kUnique | te::kQu)))
      error::fatal("two-ended: a class mixes plain stems with unique or qu stems");
  for (unsigned c = 0; c < classes.size(); ++c) {
    std::map<unsigned, std::vector<std::pair<unsigned, bool>>> byEnding{};
    const auto& [ids, keep] = classes[c];
    for (unsigned i = 0; i < ids.size(); ++i)
      byEnding[endingIds.at(std::get<1>(rows[ids[i]]))].emplace_back(ids[i], keep[i]);
    for (auto& [e, refs] : byEnding) {
      std::ranges::sort(refs, [&](const auto a, const auto b) { return order[a.first] < order[b.first]; });
      // INFO: As order() does, in order: a row with an earlier row's quality goes, and takes that place if
      //  arrayBefore puts it first (shorter ending, then the later source line).
      if (classPlain[c]) {
        std::vector<std::pair<unsigned, bool>> kept{};
        for (const auto ref : refs) {
          const auto& a = rows[ref.first];
          const auto same =
              std::ranges::find_if(kept, [&](const auto k) { return std::get<3>(rows[k.first]) == std::get<3>(a); });
          if (same == kept.end()) {
            kept.push_back(ref);
            continue;
          }
          const auto& b = rows[same->first];
          const auto la = std::get<2>(a).size(), lb = std::get<2>(b).size();
          if (la < lb || (la == lb && std::get<0>(a) > std::get<0>(b)))
            *same = ref;
          ++droppedDuplicates;
        }
        refs = std::move(kept);
      }
      std::uint64_t mask = 0;
      for (unsigned i = 0; i < refs.size(); ++i)
        if (refs[i].second && i < 64)
          mask |= std::uint64_t{1} << i;
      masks.insert(mask);
      cellOf[c * endings + e] = static_cast<unsigned>(cells.size());
      cells.push_back({static_cast<unsigned>(rowIds.size()), static_cast<unsigned>(refs.size()), mask});
      maxRows = std::max(maxRows, static_cast<unsigned>(refs.size()));
      for (const auto [id, allowed] : refs)
        rowIds.push_back(id);
    }
  }
  storage::cellOf = std::move(cellOf);
}
