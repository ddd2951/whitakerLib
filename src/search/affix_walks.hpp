#pragma once

// INFO: Experimental affix marks. They replace text searches, never licensing, sweep groups or pass conditions.
#include "two_ended.hpp"

#include <type_traits>

namespace whitaker::affix_walks {
namespace rel = relationship;
inline constexpr two_ended::Table<rel::Node, rel::Section::AffixBack> kBack{};
inline constexpr two_ended::Table<rel::Node, rel::Section::PrefixFront> kPrefixes{};
inline constexpr two_ended::Table<rel::Node, rel::Section::SuffixBack> kSuffixes{};
inline constexpr two_ended::Table<std::uint16_t, rel::Section::AddonIds> kAddonIds{};
inline constexpr two_ended::Table<rel::EndingRecord, rel::Section::Endings> kEndings{};

struct Marks {
  static constexpr unsigned kCuts = facts::kMaxWordCharacters + 1;
  std::array<std::array<rel::Run, kCuts>, kCuts> stemAt{};
  std::array<std::uint32_t, kCuts> stemCuts{};
  std::uint32_t walked{};
  // INFO: An add-on's id is its place in the one table of add-ons, so marks are bits a loop can visit in table order.
  using Bits = std::array<std::uint64_t, (rel::kAffixAddonCount + 63) / 64>;
  Bits prefixes{};
  Bits suffixes{};
  std::array<std::uint32_t, rel::kAffixAddonCount> suffixCuts{};

  // INFO: The first marked id at or after from, or kAffixAddonCount.
  [[nodiscard]] static unsigned next(const Bits& bits, const unsigned from) {
    for (unsigned w = from / 64; w < bits.size(); ++w)
      if (const std::uint64_t m = bits[w] & (w == from / 64 ? ~std::uint64_t{0} << (from % 64) : ~std::uint64_t{0}))
        return w * 64 + static_cast<unsigned>(std::countr_zero(m));
    return rel::kAffixAddonCount;
  }

  template <bool Reverse> static void walk(auto nodes, const std::string_view word, auto&& terminal) {
    auto node = nodes[0];
    for (unsigned i = 0;; ++i) {
      const unsigned cut = Reverse ? static_cast<unsigned>(word.size()) - i : i;
      if (!rel::empty(node.run))
        terminal(cut, node.run);
      if (i == word.size())
        break;
      const unsigned c = static_cast<unsigned char>(word[Reverse ? cut - 1 : i]) - unsigned('a');
      if (c >= facts::kLetterCount)
        break;
      const auto bit = std::uint32_t{1} << c;
      if (!(node.mask & bit))
        break;
      node = nodes[node.first + static_cast<unsigned>(std::popcount(node.mask & (bit - 1)))];
    }
  }

  void begin(const std::string_view word) {
    walked = 0;
    prefixes.fill(0);
    // NOTE: Only the counters marked last time are non-zero.
    for (unsigned id = next(suffixes, 0); id < rel::kAffixAddonCount; id = next(suffixes, id + 1))
      suffixCuts[id] = 0;
    suffixes.fill(0);
    walk<false>(kPrefixes, word, [&](unsigned, const rel::Run run) {
      for (auto i = run.first; i < rel::end(run); ++i)
        prefixes[kAddonIds[i] / 64] |= std::uint64_t{1} << (kAddonIds[i] % 64);
    });
  }

  void endings(const std::string_view word, auto&& split) {
    // INFO: The backward walk sees empty then successively longer endings, exactly the old split order.
    walk<true>(kBack, word, [&](const unsigned cut, const rel::Run run) {
      if (!cut)
        return;
      split(cut, rel::inflections(kEndings[run.first]));
      walk<true>(kSuffixes, word.substr(0, cut), [&](const unsigned base, const rel::Run addons) {
        if (!base)
          return;
        for (auto i = addons.first; i < rel::end(addons); ++i) {
          suffixCuts[kAddonIds[i]] |= std::uint32_t{1} << cut;
          suffixes[kAddonIds[i] / 64] |= std::uint64_t{1} << (kAddonIds[i] % 64);
        }
      });
    });
  }

  [[nodiscard]] rel::Run stems(const std::string_view word, const unsigned start, const unsigned length) {
    if (!(walked & (std::uint32_t{1} << start))) {
      walked |= std::uint32_t{1} << start;
      stemCuts[start] = 0;
      two_ended::walkFront(word.substr(start), [&](const unsigned cut, const std::uint32_t index) {
        stemCuts[start] |= std::uint32_t{1} << cut;
        stemAt[start][cut] = two_ended::frontRun(index);
      });
    }
    return (stemCuts[start] & (std::uint32_t{1} << length)) ? stemAt[start][length] : rel::Run{};
  }
};
static_assert(std::is_trivially_copyable_v<Marks>);
} // namespace whitaker::affix_walks
