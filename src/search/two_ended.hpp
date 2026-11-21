#pragma once

// INFO: Source-derived dictionary pass, intersecting front and back cuts.
#include "relationship_image.hpp"
#include "two_ended_schema.hpp"
#include "two_ended_bounds.hpp"
#include "image.hpp"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <span>
#include <vector>
#include <type_traits>

namespace whitaker::two_ended {

static_assert(relationship::kNodeBytes == 8);
template <typename T, relationship::Section S> struct Table {
  [[nodiscard, gnu::always_inline]] inline T operator[](const std::size_t i) const {
    const auto* const p = relationship::imageRecord(S, i);
    if constexpr (std::is_same_v<T, Node>)
      return decode(relationship::readWord(p, relationship::kNodeBytes));
    else if constexpr (std::is_integral_v<T>)
      return relationship::readField<T>(p);
    else
      return relationship::readRecord<T>(p);
  }
  static constexpr relationship::Section kSection = S;
  [[nodiscard, gnu::always_inline]] static constexpr std::uint32_t maskOf(const std::uint64_t bits) {
    return static_cast<std::uint32_t>(bits & ((std::uint64_t{1} << relationship::kNodeMaskBits) - 1));
  }
  [[nodiscard, gnu::always_inline]] static constexpr std::uint32_t firstOf(const std::uint64_t bits) {
    using namespace relationship;
    return static_cast<std::uint32_t>((bits >> kNodeMaskBits) & ((std::uint64_t{1} << kNodeFirstBits) - 1));
  }
  [[nodiscard, gnu::always_inline]] static constexpr std::uint32_t countOf(const std::uint64_t bits) {
    using namespace relationship;
    return static_cast<std::uint32_t>(bits >> (kNodeMaskBits + kNodeFirstBits + kNodeRunBits));
  }
  [[nodiscard, gnu::always_inline]] static constexpr relationship::Run runOf(const std::uint64_t bits) {
    using namespace relationship;
    return {static_cast<std::uint32_t>((bits >> (kNodeMaskBits + kNodeFirstBits)) &
                                       ((std::uint64_t{1} << kNodeRunBits) - 1)),
            countOf(bits)};
  }
  [[nodiscard, gnu::always_inline]] static inline Node decode(const std::uint64_t bits) {
    using namespace relationship;
    Node n{};
    // INFO: No node read here uses the folded-away letters' bits (embedded_image.cpp proves it), so the mask is the
    //  letters and the count field is the whole count.
    n.mask = maskOf(bits);
    n.first = firstOf(bits);
    n.run = runOf(bits);
    return n;
  }
  [[nodiscard]] std::uint64_t size() const { return relationship::frameSection(S).count; }
};
inline constexpr Table<Node, relationship::Section::Back> kBack{};
[[nodiscard, gnu::always_inline]] inline relationship::Run headRun(const std::uint16_t packed) {
  return {packed & ((1u << kHeadRunFirstBits) - 1), static_cast<std::uint32_t>(packed >> kHeadRunFirstBits)};
}
struct Stems {
  [[nodiscard, gnu::always_inline]] inline Stem operator[](const std::size_t i) const {
    const auto r =
        relationship::readRecord<relationship::StemRecord>(relationship::imageRecord(relationship::Section::Stems, i));
    return {r.dictionary, r.klass, relationship::column(r), relationship::flags(r)};
  }
  [[nodiscard]] std::uint64_t size() const { return relationship::frameSection(relationship::Section::Stems).count; }
};
inline constexpr Stems kStems{};
inline constexpr Table<Rule, relationship::Section::Rules> kRules{};
inline constexpr Table<Cell, relationship::Section::Cells> kCells{};
inline constexpr Table<relationship::CellIndex, relationship::Section::CellIndex> kCellIndex{};
inline constexpr Table<std::uint16_t, relationship::Section::CellOf> kCellOf{};
inline constexpr Table<std::uint16_t, relationship::Section::RowIds> kRowIds{};

[[nodiscard]] inline unsigned cellId(const unsigned klass, const unsigned ending) {
  const auto block = kCellIndex[klass * ((kEndings + 31) / 32) + ending / 32];
  const auto bit = std::uint32_t{1} << (ending % 32);
  return (block.mask & bit) ? kCellOf[block.first + static_cast<unsigned>(std::popcount(block.mask & (bit - 1)))] : 0;
}

[[nodiscard, gnu::always_inline]] inline std::uint64_t frontSlotAt(const unsigned char* const base,
                                                                   const std::size_t i) {
  std::uint64_t bits;
  std::memcpy(&bits, base + i * relationship::kNodeBytes, sizeof bits);
  return bits;
}
[[nodiscard, gnu::always_inline]] inline relationship::Run frontRun(const std::uint32_t index) {
  const auto* const p = relationship::imageRecord(relationship::Section::FrontRuns, index);
  const auto first = relationship::read16(p);
  return {first, static_cast<std::uint32_t>(relationship::read16(p + 2) - first)};
}
// INFO: The lookup's front walk without a branch on stem ends: every step stores its index, and the stem bit goes into
//  the returned cut marks. at[cut] means something only where the mark is set.
[[gnu::always_inline]] inline std::uint32_t
walkFrontMarks(const std::string_view word, std::array<std::uint32_t, facts::kMaxWordCharacters + 1>& at) {
  using namespace relationship;
  const auto* const base = recordAt(*sImage, Section::Front, 0);
  std::uint64_t slot = frontSlotAt(base, 0);
  std::uint32_t index = 0, marks = 0;
  for (unsigned i = 0;; ++i) {
    const auto stem = static_cast<std::uint32_t>(slot >> (kFrontMaskBits + kFrontFirstBits + kFrontOffsetBits)) & 1u;
    marks |= stem << i;
    at[i] = index;
    if (i == word.size())
      return marks;
    const unsigned c = static_cast<unsigned>(static_cast<unsigned char>(word[i])) - unsigned('a');
    if (c >= 26)
      return marks;
    const auto mask = static_cast<std::uint32_t>(slot & ((std::uint64_t{1} << kFrontMaskBits) - 1));
    const auto bit = std::uint32_t{1} << c;
    if (!(mask & bit))
      return marks;
    const auto first =
        static_cast<std::uint32_t>((slot >> kFrontMaskBits) & ((std::uint64_t{1} << kFrontFirstBits) - 1));
    slot = frontSlotAt(base, first + static_cast<unsigned>(std::popcount(mask & (bit - 1))));
    index += stem + static_cast<std::uint32_t>((slot >> (kFrontMaskBits + kFrontFirstBits)) &
                                               ((std::uint64_t{1} << kFrontOffsetBits) - 1));
  }
}
// INFO: The lookup's front walk: the first two steps from kFrontHead in one load (gen walks them for every pair of
//  letters), then walkFrontMarks' loop without a branch on stem ends. The affix walks keep walkFront.
[[gnu::always_inline]] inline std::uint32_t
walkFrontHeadMarks(const std::string_view word, std::array<std::uint32_t, facts::kMaxWordCharacters + 1>& at) {
  using namespace relationship;
  if (word.size() < 2)
    return walkFrontMarks(word, at);
  const auto& h = kFrontHead[(static_cast<unsigned>(static_cast<unsigned char>(word[0])) - unsigned('a')) * 26 +
                             static_cast<unsigned>(static_cast<unsigned char>(word[1])) - unsigned('a')];
  std::uint32_t marks = h.marks;
  at[0] = 0;
  at[1] = h.at1;
  at[2] = h.at2;
  if (h.depth < 2)
    return marks;
  const auto* const base = recordAt(*sImage, Section::Front, 0);
  std::uint64_t slot = h.slot;
  std::uint32_t index = h.at2;
  for (unsigned i = 2;; ++i) {
    const auto stem = static_cast<std::uint32_t>(slot >> (kFrontMaskBits + kFrontFirstBits + kFrontOffsetBits)) & 1u;
    marks |= stem << i;
    at[i] = index;
    if (i == word.size())
      return marks;
    const unsigned c = static_cast<unsigned>(static_cast<unsigned char>(word[i])) - unsigned('a');
    if (c >= 26)
      return marks;
    const auto mask = static_cast<std::uint32_t>(slot & ((std::uint64_t{1} << kFrontMaskBits) - 1));
    const auto bit = std::uint32_t{1} << c;
    if (!(mask & bit))
      return marks;
    const auto first =
        static_cast<std::uint32_t>((slot >> kFrontMaskBits) & ((std::uint64_t{1} << kFrontFirstBits) - 1));
    slot = frontSlotAt(base, first + static_cast<unsigned>(std::popcount(mask & (bit - 1))));
    index += stem + static_cast<std::uint32_t>((slot >> (kFrontMaskBits + kFrontFirstBits)) &
                                               ((std::uint64_t{1} << kFrontOffsetBits) - 1));
  }
}
// INFO: Calls found(cut, index) at each stem the word starts with; frontRun(index) gives its rows.
template <typename Found> [[gnu::always_inline]] inline void walkFront(const std::string_view word, Found&& found) {
  using namespace relationship;
  const auto* const base = imageRecord(Section::Front, 0);
  std::uint64_t slot = frontSlotAt(base, 0);
  std::uint32_t index = 0;
  for (unsigned i = 0;; ++i) {
    const auto stem = static_cast<std::uint32_t>(slot >> (kFrontMaskBits + kFrontFirstBits + kFrontOffsetBits)) & 1u;
    if (stem)
      found(i, index);
    if (i == word.size())
      return;
    const unsigned c = static_cast<unsigned>(static_cast<unsigned char>(word[i])) - unsigned('a');
    if (c >= 26)
      return;
    const auto mask = static_cast<std::uint32_t>(slot & ((std::uint64_t{1} << kFrontMaskBits) - 1));
    const auto bit = std::uint32_t{1} << c;
    if (!(mask & bit))
      return;
    const auto first =
        static_cast<std::uint32_t>((slot >> kFrontMaskBits) & ((std::uint64_t{1} << kFrontFirstBits) - 1));
    slot = frontSlotAt(base, first + static_cast<unsigned>(std::popcount(mask & (bit - 1))));
    index += stem + static_cast<std::uint32_t>((slot >> (kFrontMaskBits + kFrontFirstBits)) &
                                               ((std::uint64_t{1} << kFrontOffsetBits) - 1));
  }
}

// INFO: walkFront the other way: the letters of the stem numbered `target` into `letters`, folded as the lookup
//  stores them.
inline void walkFront(const std::uint32_t target, const std::span<char, facts::kMaxWordCharacters> letters) {
  using namespace relationship;
  const auto* const base = imageRecord(Section::Front, 0);
  std::uint64_t slot = frontSlotAt(base, 0);
  std::uint32_t index = 0;
  for (unsigned i = 0;; ++i) {
    const auto stem = static_cast<std::uint32_t>(slot >> (kFrontMaskBits + kFrontFirstBits + kFrontOffsetBits)) & 1u;
    if (stem && index == target)
      return;
    assert(i < letters.size());
    const auto mask = static_cast<std::uint32_t>(slot & ((std::uint64_t{1} << kFrontMaskBits) - 1));
    const auto first =
        static_cast<std::uint32_t>((slot >> kFrontMaskBits) & ((std::uint64_t{1} << kFrontFirstBits) - 1));
    std::uint64_t next = 0;
    std::uint32_t nextIndex = 0;
    unsigned letter = 26, rank = 0;
    for (auto rest = mask; rest; rest &= rest - 1, ++rank) {
      const auto child = frontSlotAt(base, first + rank);
      const auto at = index + stem +
                      static_cast<std::uint32_t>((child >> (kFrontMaskBits + kFrontFirstBits)) &
                                                 ((std::uint64_t{1} << kFrontOffsetBits) - 1));
      if (at > target)
        break;
      next = child;
      nextIndex = at;
      letter = static_cast<unsigned>(std::countr_zero(rest));
    }
    assert(letter < 26);
    letters[i] = static_cast<char>('a' + letter);
    slot = next;
    index = nextIndex;
  }
}

// INFO: A stem walkFront spelled, as its source spells it: the kth u or i back to v or j where bit k of `marks` says
//  so, then its leading capitals (markStemLetters in gen).
inline void unfold(const std::span<char, facts::kMaxWordCharacters> letters, const std::uint16_t marks) {
  unsigned folds = 0;
  for (char& c : letters) {
    if (c != 'u' && c != 'i')
      continue;
    if ((unsigned{marks} >> folds) & 1u)
      c = c == 'u' ? 'v' : 'j';
    ++folds;
  }
  for (unsigned i = 0; i < (unsigned{marks} >> 8) && i < letters.size(); ++i)
    letters[i] = static_cast<char>(letters[i] - 'a' + 'A');
}

enum class Stage : int { walks, candidates, ordering, adverbs, sweep, compress, append };
struct Candidate {
  std::uint32_t stem{};
  std::uint16_t rule{};
  bool allowed{};
  latin::Age entryAge{};
  latin::Frequency entryFrequency{};
};
static_assert(std::is_trivially_copyable_v<Candidate>);

// INFO: Gen bounds every unswept candidate from the source, plus at most one correction per candidate.
//  Fixed scratch keeps allocation failure on Result's checked paths, including the first dictionary lookup.
template <std::size_t N> struct Candidates {
  std::array<Candidate, N> items{};
  std::size_t count{};
  auto begin() { return items.data(); }
  auto end() { return begin() + count; }
  auto begin() const { return items.data(); }
  auto end() const { return begin() + count; }
  std::size_t size() const { return count; }
  bool empty() const { return count == 0; }
  Candidate& front() { return items[0]; }
  Candidate& operator[](const std::size_t i) { return items[i]; }
  const Candidate& operator[](const std::size_t i) const { return items[i]; }
  void clear() { count = 0; }
  void push_back(const Candidate value) {
    assert(count < N);
    items[count++] = value;
  }
  void erase(Candidate* const first, Candidate* const last) {
    count = static_cast<std::size_t>(std::move(last, end(), first) - begin());
  }
  void erase(Candidate* const at) { erase(at, at + 1); }
};

static_assert(std::is_trivially_copyable_v<Candidates<kCandidateCapacity>>);
static_assert(std::is_trivially_copyable_v<Candidates<2 * kCandidateCapacity>>);

struct Lookup {
  Candidates<2 * kCandidateCapacity> candidates{};
  Candidates<kCandidateCapacity> corrections{};
  bool trimmed{}, reordered{};

  static std::uint32_t walk(auto nodes, const std::string_view word, const bool reverse,
                            std::array<relationship::Run, facts::kMaxWordCharacters + 1>& at,
                            const std::uint32_t live = ~std::uint32_t{0}) {
    std::uint32_t marks = 0;
    // INFO: One image load per walk; a node's word is a plain little-endian read.
    const auto* const base = relationship::imageRecord(std::remove_cvref_t<decltype(nodes)>::kSection, 0);
    using Nodes = std::remove_cvref_t<decltype(nodes)>;
    const auto at_ = [base](const std::size_t i) {
      std::uint64_t bits;
      std::memcpy(&bits, base + i * relationship::kNodeBytes, sizeof bits);
      return bits;
    };
    // INFO: A step keeps the node's raw word; the run is unpacked only at a live cut that has one.
    std::uint64_t n = at_(0);
    for (unsigned i = 0;; ++i) {
      const unsigned cut = reverse ? static_cast<unsigned>(word.size()) - i : i;
      if (Nodes::countOf(n) && (live & (std::uint32_t{1} << cut))) {
        marks |= 1u << cut;
        at[cut] = Nodes::runOf(n);
      }
      if (i == word.size())
        break;
      // INFO: Further reverse cuts are strictly below this one; forward cuts are strictly above it.
      if (reverse ? !(live & ((std::uint32_t{1} << cut) - 1)) : !(live >> (cut + 1)))
        break;
      const unsigned c = static_cast<unsigned>(static_cast<unsigned char>(word[reverse ? cut - 1 : i])) - unsigned('a');
      const auto bit = 1u << c;
      const auto mask = Nodes::maskOf(n);
      if (!(mask & bit))
        break;
      n = at_(Nodes::firstOf(n) + static_cast<unsigned>(std::popcount(mask & (bit - 1))));
    }
    return marks;
  }

  // INFO: The back walk with its first two steps from kBackHead (by the word's last two letters). Those steps run even
  //  where walk() would stop early for want of live cuts; their marks are masked by live all the same.
  static std::uint32_t walkBackHead(const std::string_view word,
                                    std::array<relationship::Run, facts::kMaxWordCharacters + 1>& at,
                                    const std::uint32_t live) {
    if (word.size() < 2)
      return walk(kBack, word, true, at, live);
    const unsigned size = static_cast<unsigned>(word.size());
    const auto& h = kBackHead[(static_cast<unsigned>(static_cast<unsigned char>(word[size - 1])) - unsigned('a')) * 26 +
                              static_cast<unsigned>(static_cast<unsigned char>(word[size - 2])) - unsigned('a')];
    std::uint32_t marks = 0;
    for (unsigned k = 0; k <= h.depth; ++k) {
      const auto run = headRun(h.run[k]);
      const unsigned cut = size - k;
      if (!relationship::empty(run) && (live & (std::uint32_t{1} << cut))) {
        marks |= 1u << cut;
        at[cut] = run;
      }
    }
    if (h.depth < 2)
      return marks;
    const auto* const base = relationship::recordAt(*relationship::sImage, relationship::Section::Back, 0);
    Node n{h.mask, h.first, headRun(h.run[2])};
    for (unsigned i = 2;; ++i) {
      const unsigned cut = size - i;
      if (i != 2 && !relationship::empty(n.run) && (live & (std::uint32_t{1} << cut))) {
        marks |= 1u << cut;
        at[cut] = n.run;
      }
      if (i == size)
        break;
      if (!(live & ((std::uint32_t{1} << cut) - 1)))
        break;
      const unsigned c = static_cast<unsigned>(static_cast<unsigned char>(word[cut - 1])) - unsigned('a');
      const auto bit = 1u << c;
      if (!(n.mask & bit))
        break;
      std::uint64_t bits;
      std::memcpy(
          &bits, base + (n.first + static_cast<unsigned>(std::popcount(n.mask & (bit - 1)))) * relationship::kNodeBytes,
          sizeof bits);
      n = std::remove_cvref_t<decltype(kBack)>::decode(bits);
    }
    return marks;
  }

  // INFO: Within an entry, resetting a pronoun's kind changes the same constant on every row; equality is unchanged.
  static bool sameGrammar(const Candidate a, const Candidate b) {
    return kStems[a.stem].dictionary == kStems[b.stem].dictionary && kRules[a.rule].quality == kRules[b.rule].quality;
  }
  static bool before(const Candidate a, const Candidate b) {
    const auto& sa = kStems[a.stem];
    const auto& sb = kStems[b.stem];
    if ((sa.flags & kUnique) != (sb.flags & kUnique))
      return (sa.flags & kUnique) != 0;
    if (sa.dictionary != sb.dictionary)
      return sa.dictionary < sb.dictionary;
    return kRules[a.rule].order < kRules[b.rule].order;
  }
  static bool arrayBefore(const Candidate a, const Candidate b) {
    const auto& sa = kStems[a.stem];
    const auto& sb = kStems[b.stem];
    const auto& ra = kRules[a.rule];
    const auto& rb = kRules[b.rule];
    if (sa.dictionary != sb.dictionary)
      return sa.dictionary < sb.dictionary;
    if (ra.endingLength != rb.endingLength)
      return ra.endingLength < rb.endingLength;
    if (ra.quality != rb.quality)
      return ra.arrayOrder < rb.arrayOrder;
    return ra.source > rb.source;
  }

  void order() {
    // INFO: Gen proves equal qualities never have different ending lengths for the same spelling and entry.
    for (unsigned i = 0; i < candidates.size();) {
      const auto a = candidates[i];
      bool duplicate = false;
      if (!(kStems[a.stem].flags & (kUnique | kQu)))
        for (unsigned j = 0; j < i; ++j)
          if (sameGrammar(a, candidates[j])) {
            if (arrayBefore(a, candidates[j]))
              candidates[j] = a;
            duplicate = true;
            break;
          }
      if (duplicate)
        candidates.erase(candidates.begin() + static_cast<std::ptrdiff_t>(i));
      else
        ++i;
    }
    reordered = !std::ranges::is_sorted(candidates, before);
    if (reordered)
      std::stable_sort(candidates.begin(), candidates.end(), before);
  }

  void adverbs() {
    if (std::ranges::any_of(candidates, [](const Candidate a) {
          return kRules[a.rule].reading.rex == std::to_underlying(latin::Part::adverb) * 2;
        }))
      return;
    for (const auto a : candidates) {
      const auto fix = kRules[a.rule].correction;
      if (fix < 0)
        continue;
      // INFO: Fix_Adverb selects the last qualifying source in Order_Stems order, once per entry and degree.
      const auto old = std::ranges::find_if(corrections, [&](const Candidate b) {
        return kStems[a.stem].dictionary == kStems[b.stem].dictionary &&
               kRules[static_cast<std::uint16_t>(fix)].quality ==
                   kRules[static_cast<std::uint16_t>(kRules[b.rule].correction)].quality;
      });
      if (old == corrections.end())
        corrections.push_back(a);
      else if (arrayBefore(*old, a))
        *old = a;
    }
    for (auto& a : corrections) {
      a.rule = static_cast<std::uint16_t>(kRules[a.rule].correction);
      a.allowed = true;
    }
    if (!std::ranges::is_sorted(corrections, before))
      std::stable_sort(corrections.begin(), corrections.end(), before);
  }

  void sweep(auto& group) {
    facts::RarityEvidence evidence;
    for (const auto a : group)
      if (!(kStems[a.stem].flags & kUnique)) {
        const auto& r = kRules[a.rule];
        evidence.count(r.age, a.entryAge, r.frequency, a.entryFrequency);
      }
    group.erase(std::remove_if(group.begin(), group.end(),
                               [&](const Candidate a) {
                                 const auto& r = kRules[a.rule];
                                 const bool drop = !a.allowed || evidence.drops(r.age, r.frequency);
                                 trimmed = trimmed || drop;
                                 return drop;
                               }),
                group.end());
  }

  template <Stage Stop = Stage::append>
  std::uint64_t lookup(const std::string_view word, std::vector<latin::packed::Reading>& out) {
    candidates.clear();
    corrections.clear();
    trimmed = reordered = false;
    if (word.empty() || word.size() > facts::kMaxWordCharacters)
      return 0;
    char folded[facts::kMaxWordCharacters];
    for (unsigned i = 0; i < word.size(); ++i) {
      folded[i] = facts::foldLetter(word[i]);
      if (folded[i] < 'a' || folded[i] > 'z')
        return 0;
    }
    const std::string_view text(folded, word.size());
    std::array<relationship::Run, facts::kMaxWordCharacters + 1> endingAt;
    std::array<std::uint32_t, facts::kMaxWordCharacters + 1> stemAt{};
    std::uint32_t cuts = 0;
    cuts = walkFrontHeadMarks(text, stemAt);
    // INFO: A failed front walk needs no ending walk, stems or cells. Stop the reverse walk at the last live cut.
    if (cuts)
      cuts &= walkBackHead(text, endingAt, cuts);
    if constexpr (Stop == Stage::walks)
      return cuts;
    // INFO: Gen drops duplicates inside a cell and sorts its rows. Candidates from one cut whose stems rise strictly by
    //  (unique first, entry) are then in order with no entry twice, so order() has nothing to do.
    bool ordered = true;
    std::uint32_t lastKey = 0, lastCut = ~std::uint32_t{0};
    while (cuts) {
      const unsigned cut = static_cast<unsigned>(std::countr_zero(cuts));
      cuts &= cuts - 1;
      const auto stems = frontRun(stemAt[cut]);
      for (unsigned s = stems.first; s < relationship::end(stems); ++s) {
        const auto& stem = kStems[s];
        // INFO: Gen orders all plain members before affix-only members in each shared stem run.
        if (stem.klass == relationship::kNoLookupClass)
          break;
        // INFO: Gen makes cell 0 the only empty cell, so an ending the class lacks needs no cell read.
        const unsigned id = cellId(stem.klass, endingAt[cut].first);
        if (id == 0)
          continue;
        const auto& cell = kCells[id];
        const std::uint32_t key = ((stem.flags & kUnique) ? 0u : 1u << 16) | stem.dictionary;
        ordered = ordered && (lastCut == ~std::uint32_t{0} || lastCut == cut) && key > lastKey;
        lastKey = key;
        lastCut = cut;
        const auto entry = relationship::dictionary(stem.dictionary);
        const auto keep = cell.keep;
        for (unsigned i = 0; i < cell.count; ++i)
          candidates.push_back(
              {s, kRowIds[cell.first + i], (keep & (relationship::KeepMask{1} << i)) != 0, entry.age, entry.frequency});
      }
    }
    if constexpr (Stop == Stage::candidates)
      return candidates.size();
    if (candidates.empty())
      return 0;
    if (!ordered)
      order();
    if constexpr (Stop == Stage::ordering)
      return candidates.size();
    adverbs();
    if constexpr (Stop == Stage::adverbs)
      return candidates.size() + corrections.size();
    sweep(candidates);
    sweep(corrections);
    for (const auto a : corrections)
      candidates.push_back(a);
    if constexpr (Stop == Stage::sweep)
      return candidates.size();
    // INFO: Gen proves compression cannot remove a candidate after ordering and correction deduplication.
    if constexpr (Stop == Stage::compress)
      return candidates.size();
    for (const auto a : candidates) {
      const auto g = kRules[a.rule].reading;
      latin::packed::Reading r{
          latin::packed::headChange(g.rex), latin::packed::headPart(g.rex), 0, 0, 0, {g.g[0], g.g[1], g.g[2]}, 0};
      const auto& s = kStems[a.stem];
      r.entry = s.dictionary;
      r.stem = s.column;
      out.push_back(r);
    }
    return candidates.size();
  }
};
static_assert(std::is_trivially_copyable_v<Lookup>);

// INFO: Enumerate independently from the saved reference list, including spellings whose readings were all swept.
[[nodiscard]] inline std::vector<std::string> spellings(const bool includeTrimmed) {
  using NamedRun = std::pair<std::string, relationship::Run>;
  const auto names = [](auto nodes, const bool reverse) {
    std::vector<NamedRun> out{};
    std::string text{};
    const auto visit = [&](auto&& self, const unsigned at) -> void {
      const auto n = nodes[at];
      if (!relationship::empty(n.run))
        out.emplace_back(reverse ? std::string(text.rbegin(), text.rend()) : text, n.run);
      unsigned next = n.first;
      for (unsigned c = 0; c < 26; ++c)
        if (n.mask & (std::uint32_t{1} << c)) {
          text.push_back(static_cast<char>('a' + c));
          self(self, next++);
          text.pop_back();
        }
    };
    visit(visit, 0);
    return out;
  };
  std::vector<NamedRun> fronts{};
  {
    using namespace relationship;
    const auto* const base = imageRecord(Section::Front, 0);
    std::string text{};
    const auto visit = [&](auto&& self, const std::size_t at, const std::uint32_t index) -> void {
      const auto slot = frontSlotAt(base, at);
      const auto stem = static_cast<std::uint32_t>(slot >> (kFrontMaskBits + kFrontFirstBits + kFrontOffsetBits)) & 1u;
      if (stem)
        fronts.emplace_back(text, frontRun(index));
      const auto mask = static_cast<std::uint32_t>(slot & ((std::uint64_t{1} << kFrontMaskBits) - 1));
      std::size_t next = (slot >> kFrontMaskBits) & ((std::uint64_t{1} << kFrontFirstBits) - 1);
      for (unsigned c = 0; c < 26; ++c)
        if (mask & (std::uint32_t{1} << c)) {
          const auto child = frontSlotAt(base, next);
          text.push_back(static_cast<char>('a' + c));
          self(self, next++,
               index + stem +
                   static_cast<std::uint32_t>((child >> (kFrontMaskBits + kFrontFirstBits)) &
                                              ((std::uint64_t{1} << kFrontOffsetBits) - 1)));
          text.pop_back();
        }
    };
    visit(visit, 0, 0);
  }
  const auto backs = names(kBack, true);
  Lookup lookup{};
  std::vector<latin::packed::Reading> unused{};
  std::vector<std::string> out{};
  for (const auto& [stem, run] : fronts)
    for (const auto& [ending, end] : backs) {
      bool live = false;
      for (auto i = run.first; i < relationship::end(run) && !live; ++i) {
        const auto row = kStems[i];
        if (row.klass == relationship::kNoLookupClass)
          break;
        live = cellId(row.klass, end.first) != 0;
      }
      if (!live)
        continue;
      std::string word = stem + ending;
      const auto count = lookup.lookup<Stage::compress>(word, unused);
      if (count || (includeTrimmed && lookup.trimmed))
        out.push_back(std::move(word));
    }
  std::ranges::sort(out);
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

} // namespace whitaker::two_ended
