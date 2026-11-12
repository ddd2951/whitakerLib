#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "facts.hpp"
#include "source/schemes.hpp"
#include "storage/back.hpp"
#include "storage/front.hpp"
#include "storage/lookup_metadata.hpp"
#include "storage/max_rows.hpp"
#include "storage/cells.hpp"
#include "storage/row_ids.hpp"
#include "storage/rows.hpp"
#include "storage/classes.hpp"
#include "storage/unique_base.hpp"
#include "storage/mixed.hpp"
#include "storage/dropped_duplicates.hpp"
#include "storage/compression_duplicates.hpp"
#include <algorithm>
#include <bit>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::deriveLookupBounds() {
  using storage::back;
  using storage::cells;
  using storage::classes;
  using storage::compressionDuplicates;
  using storage::droppedDuplicates;
  using storage::front;
  using storage::maxRows;
  using storage::mixed;
  using storage::out;
  using storage::rowIds;
  using storage::rows;
  using storage::uniqueBase;
  unsigned maximumFirst = 0, maximumRun = 0, maximumCount = 0;
  std::uint32_t frontLetters = 0, backLetters = 0;
  for (const auto* const nodes : {&front, &back})
    for (const auto& n : *nodes) {
      maximumFirst = std::max(maximumFirst, n.first);
      maximumRun = std::max(maximumRun, n.run.first);
      maximumCount = std::max(maximumCount, n.run.count);
    }
  for (const auto& n : front)
    frontLetters |= n.mask;
  for (const auto& n : back)
    backLetters |= n.mask;
  if ((frontLetters | backLetters) & facts::kFoldedAwayMask)
    error::fatal("two-ended: source trie uses a folded-away letter");
  const auto firstBits = static_cast<unsigned>(std::bit_width(maximumFirst)),
             runBits = static_cast<unsigned>(std::bit_width(maximumRun)),
             countWidth = static_cast<unsigned>(std::bit_width(maximumCount)),
             payloadWidth = static_cast<unsigned>(std::popcount(facts::kFoldedAwayMask)),
             countBits = countWidth > payloadWidth ? countWidth - payloadWidth : 0;
  const unsigned maskBits = maxRows <= 8 ? 8 : maxRows <= 16 ? 16 : maxRows <= 32 ? 32 : 64;
  out << "// Generated from latin and the source files; do not edit.\n#pragma once\n#include <cstdint>\n#include "
         "<bit>\n#include \"facts.hpp\"\n";
  out << "namespace whitaker::relationship {\n"
      << "inline constexpr unsigned kNodeMaskBits = " << facts::kLetterCount << ";\n"
      << "inline constexpr unsigned kNodeFirstBits = " << firstBits << ";\n"
      << "inline constexpr unsigned kNodeRunBits = " << runBits << ";\n"
      << "inline constexpr unsigned kNodeCountBits = " << countBits << ";\n"
      << "inline constexpr unsigned kNodeBytes = " << (facts::kLetterCount + firstBits + runBits + countBits + 7) / 8
      << ";\nstatic_assert(kNodeMaskBits + kNodeFirstBits + kNodeRunBits + kNodeCountBits <= 64);\n}\n";
  out << "static_assert((" << frontLetters
      << "u & facts::kFoldedAwayMask) == 0, \"front uses a folded-away letter\");\n";
  out << "static_assert((" << backLetters << "u & facts::kFoldedAwayMask) == 0, \"back uses a folded-away letter\");\n";
  out << "static_assert(" << maximumCount
      << "ull < (1ull << (whitaker::relationship::kNodeCountBits + std::popcount(facts::kFoldedAwayMask))));\n";
  out << "namespace whitaker::relationship { using KeepMask = std::uint" << maskBits << "_t; }\n";
  out << "static_assert(" << maxRows << " <= " << maskBits << ");\n";
  out << "static_assert(" << front.size() << "ull <= 4294967295ull);\nstatic_assert(" << back.size()
      << "ull <= 4294967295ull);\n";
  out << "static_assert(" << maxRows << " <= 64);\nstatic_assert(" << cells.size() << " <= 65536);\nstatic_assert("
      << rowIds.size() << " <= 65536);\nstatic_assert(" << rows.size() << " <= 32768);\nstatic_assert("
      << classes.size() << " < 65536);\nstatic_assert(" << uniqueBase + source::scheme::uniques::kEntriesPerFile
      << " <= 65536);\n";
  out << "static_assert(" << mixed << " == 0, \"mixed ending lengths require Array_Stems ordering\");\n";
  out << "// " << droppedDuplicates << " duplicate rows dropped inside cells\n";
  out << "static_assert(" << compressionDuplicates
      << " == 0, \"compression requires unique exceptional qualities\");\n";
}
