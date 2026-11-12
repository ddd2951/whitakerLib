#include "execution_steps/steps.hpp"

#include "storage/lines.hpp"
#include "storage/records.hpp"

#include <cstddef>
#include <meta>

void execution_steps::uniques::indexUniquesRecords() {
  using storage::lines;
  using storage::records;
  constexpr auto file = ^^source::scheme::uniques;
  static_assert(records<file>.size() == lines<file>.size());
  for (std::size_t at{}; at < records<file>.size(); ++at)
    records<file>[at] = at;
}
