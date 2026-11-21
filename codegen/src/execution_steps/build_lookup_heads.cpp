#include "execution_steps/steps.hpp"

#include "storage/result.hpp"
#include "storage/back.hpp"
#include "storage/lookup_metadata.hpp"
#include <bit>
#include <cassert>
#include <cstdint>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::buildLookupHeads() {
  using storage::back;
  using storage::out;
  using storage::result;
  namespace te = whitaker::two_ended;
  // INFO: The first two steps of each walk as one table load, indexed by two letters (first two for the front walk,
  //  last two for the back walk). Each entry is the walk as the lookup would run it, from the final tries.
  {
    const auto& f = result.sharedFront;
    const auto mask = [](const std::uint64_t slot) {
      return static_cast<std::uint32_t>(slot & ((1ull << rel::kFrontMaskBits) - 1));
    };
    const auto firstOf = [](const std::uint64_t slot) {
      return static_cast<std::uint32_t>((slot >> rel::kFrontMaskBits) & ((1ull << rel::kFrontFirstBits) - 1));
    };
    const auto offsetOf = [](const std::uint64_t slot) {
      return static_cast<std::uint32_t>((slot >> (rel::kFrontMaskBits + rel::kFrontFirstBits)) &
                                        ((1ull << rel::kFrontOffsetBits) - 1));
    };
    const auto stemOf = [](const std::uint64_t slot) {
      return static_cast<std::uint32_t>(slot >> (rel::kFrontMaskBits + rel::kFrontFirstBits + rel::kFrontOffsetBits)) &
             1u;
    };
    out << "namespace whitaker::two_ended {\n"
        << "struct FrontHead { std::uint64_t slot; std::uint16_t at1, at2; std::uint8_t marks, depth; };\n"
        << "inline constexpr FrontHead kFrontHead[676] = {";
    for (unsigned c0 = 0; c0 < 26; ++c0)
      for (unsigned c1 = 0; c1 < 26; ++c1) {
        std::uint64_t slot = f[0];
        std::uint32_t index = 0, marks = stemOf(slot), at[3]{}, depth = 0;
        for (const unsigned c : {c0, c1}) {
          if (!(mask(slot) & (1u << c)))
            break;
          const auto stem = stemOf(slot);
          slot = f[firstOf(slot) + static_cast<unsigned>(std::popcount(mask(slot) & ((1u << c) - 1)))];
          index += stem + offsetOf(slot);
          ++depth;
          at[depth] = index;
          marks |= stemOf(slot) << depth;
        }
        assert(at[1] <= 65535 && at[2] <= 65535);
        out << "{" << (depth == 2 ? slot : 0) << "ull, " << at[1] << ", " << at[2] << ", " << marks << ", " << depth
            << "}, ";
      }
    out << "};\n";
    out << "struct BackHead { std::uint32_t mask; std::uint16_t first; std::uint16_t run[3]; std::uint8_t depth; };\n"
        << "inline constexpr BackHead kBackHead[676] = {";
    const auto packRun = [](const te::Node n) {
      assert(n.run.first < (1u << te::kHeadRunFirstBits) && n.run.count < (1u << te::kHeadRunCountBits));
      return n.run.first | (n.run.count << te::kHeadRunFirstBits);
    };
    for (unsigned last = 0; last < 26; ++last)
      for (unsigned before = 0; before < 26; ++before) {
        te::Node n = back[0];
        unsigned run[3]{packRun(n), 0, 0}, depth = 0;
        for (const unsigned c : {last, before}) {
          if (!(n.mask & (1u << c)))
            break;
          n = back[n.first + static_cast<unsigned>(std::popcount(n.mask & ((1u << c) - 1)))];
          run[++depth] = packRun(n);
        }
        out << "{" << (depth == 2 ? n.mask : 0) << "u, " << (depth == 2 ? n.first : 0) << ", {" << run[0] << ", "
            << run[1] << ", " << run[2] << "}, " << depth << "}, ";
      }
    out << "};\n}\n";
  }
}
