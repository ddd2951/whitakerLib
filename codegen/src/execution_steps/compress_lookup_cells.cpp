#include "execution_steps/steps.hpp"

#include "storage/result.hpp"
#include "storage/cells.hpp"
#include "storage/cell_of.hpp"
#include "storage/row_ids.hpp"
#include "storage/classes.hpp"
#include "storage/endings.hpp"
#include "storage/lookup_metadata.hpp"
#include <cstdint>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::compressLookupCells() {
  using storage::cellOf;
  using storage::cells;
  using storage::classes;
  using storage::endings;
  using storage::out;
  using storage::result;
  using storage::rowIds;
  for (const auto c : cells)
    result.cells.push_back(
        {static_cast<std::uint16_t>(c.first), static_cast<std::uint8_t>(c.count), static_cast<rel::KeepMask>(c.keep)});
  for (unsigned c = 0; c < classes.size(); ++c)
    for (unsigned e = 0; e < endings; e += 32) {
      rel::CellIndex block{0, static_cast<std::uint16_t>(result.cellOf.size())};
      for (unsigned bit = 0; bit < 32 && e + bit < endings; ++bit) {
        const auto id = cellOf[c * endings + e + bit];
        if (id) {
          block.mask |= std::uint32_t{1} << bit;
          result.cellOf.push_back(static_cast<std::uint16_t>(id));
        }
      }
      result.cellIndex.push_back(block);
    }
  for (const auto r : rowIds)
    result.rowIds.push_back(static_cast<std::uint16_t>(r));
  result.metadata = out.str();
}
