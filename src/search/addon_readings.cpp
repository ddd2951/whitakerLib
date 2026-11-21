#include "search/addon_readings.hpp"
#include "per_thread.hpp"

#include "facts.hpp"
#include "parse/result.hpp"
#include "search/image.hpp"
#include "search/relationship_image.hpp"
#include "search/relationship_schema.hpp"
#include "search/affix_walks.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ranges>
#include <string_view>
#include <type_traits>

namespace whitaker {
namespace {

namespace rel = whitaker::relationship;

using Part = latin::Part;

[[nodiscard]] constexpr char lower(const char value) {
  return value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a') : value;
}

struct Query {
  std::string_view folded{};
  std::string_view lowered{};
};
static_assert(std::is_trivially_copyable_v<Query>);

struct Split {
  std::uint32_t stemLength{};
  rel::Run inflectionsOfEnding{};
};
static_assert(std::is_trivially_copyable_v<Split>);

struct AddonCandidate {
  Reading reading{};
  std::uint16_t inflection{}, addon{}, secondAddon = parse::kNoAddon;
  std::uint8_t orthLength{};
};
static_assert(std::is_trivially_copyable_v<AddonCandidate>);
struct ReadingToSweep {
  AddonCandidate candidate{};
  latin::Age entryAge{};
  latin::Frequency entryFrequency{};
  bool allowed{};
};
static_assert(std::is_trivially_copyable_v<ReadingToSweep>);

// NOTE: The library runs without exceptions, so a std::vector that cannot grow would end the caller's process. This
//  grows with realloc as newMatch does, and a failed growth is reported as out of memory.
template <typename T> class Grown {
  static_assert(std::is_trivially_copyable_v<T>);

public:
  Grown() = default;
  Grown(const Grown&) = delete;
  Grown& operator=(const Grown&) = delete;
  ~Grown() { std::free(m_items); }

  [[nodiscard]] bool push(const T& item) {
    if (m_count == m_capacity) {
      const std::size_t capacity = m_capacity == 0 ? 64 : m_capacity * 2;
      auto* const moved = static_cast<T*>(std::realloc(m_items, capacity * sizeof(T)));
      if (moved == nullptr)
        return false;
      m_items = moved;
      m_capacity = capacity;
    }
    m_items[m_count++] = item;
    return true;
  }
  void clear() { m_count = 0; }
  void eraseFrom(const T* const first) { m_count = static_cast<std::size_t>(first - m_items); }
  [[nodiscard]] std::size_t size() const { return m_count; }
  [[nodiscard]] T* begin() { return m_items; }
  [[nodiscard]] T* end() { return m_items + m_count; }
  [[nodiscard]] std::span<const T> items() const { return {m_items, m_count}; }

private:
  T* m_items = nullptr;
  std::size_t m_count = 0;
  std::size_t m_capacity = 0;
};

struct AffixScratch {
  Grown<ReadingToSweep> found{};
  // NOTE: At most one split per ending length, so the word's length bounds them.
  std::array<Split, facts::kMaxWordCharacters> splits{};
  std::array<Split, facts::kMaxWordCharacters> withoutSuffix{};
  bool trimmed = false;
  bool outOfMemory = false;
  affix_walks::Marks marks{};
};
[[nodiscard, gnu::always_inline]] inline AffixScratch& affix() { return PerThread<AffixScratch>::get(); }

using rel::StemEntry;

[[nodiscard]] auto entriesWithStem(const std::string_view stem) {
  const rel::Run stems = rel::stemRange(stem);
  return std::views::iota(stems.first, rel::end(stems)) | std::views::transform(rel::stemEntry) |
         std::views::filter([](const auto& entry) { return entry.stemKey != 255; });
}

[[nodiscard]] auto entriesWithStem(const Query& query, const std::uint32_t start, const std::uint32_t length) {
  const rel::Run stems = affix().marks.stems(query.folded, start, length);
  return std::views::iota(stems.first, rel::end(stems)) | std::views::transform(rel::stemEntry) |
         std::views::filter([](const auto& entry) { return entry.stemKey != 255; });
}

[[nodiscard]] bool isPackonOrQuPronoun(const StemEntry& entry) {
  return entry.part == Part::packon || (entry.part == Part::pronoun && rel::entry(entry.dictionary).which == 1);
}

[[nodiscard]] bool isConjugationThreeOne(const std::uint16_t rules) { return (rules & rel::kRuleConjThreeOne) != 0; }

[[nodiscard]] std::uint16_t packonNumber(const std::uint16_t rules) {
  return (rules >> rel::kRulePackonShift) & rel::kRulePackonMask;
}

[[nodiscard]] bool takesNoSuffix(const std::uint16_t rules) { return (rules & rel::kRuleAbbreviation) != 0; }

[[nodiscard]] bool takesNoPrefix(const std::uint16_t rules) {
  return (rules & (rel::kRuleAbbreviation | rel::kRuleInterjOrConj)) != 0;
}

// NOTE: The first letter as ADDONS spells it, so the capitalised II, V and X never fit a lowered word.
[[nodiscard]] bool startsWithPrefix(const Query& query, const rel::Addon& prefix) {
  const std::string_view fix = prefix.fix;
  return !fix.empty() && query.folded.size() > fix.size() && query.lowered.front() == prefix.firstLetterAsSpelled &&
         query.folded.starts_with(fix) &&
         (prefix.connectingLetter == '\0' || query.lowered[fix.size()] == prefix.connectingLetter);
}

[[nodiscard]] bool stemEndsWithSuffix(const Query& query, const std::uint32_t stemLength, const rel::Addon& suffix) {
  const std::string_view fix = suffix.fix;
  const std::string_view stem = query.folded.substr(0, stemLength);
  return !fix.empty() && stem.size() > fix.size() && stem.ends_with(fix) &&
         (suffix.connectingLetter == '\0' || query.lowered[stem.size() - fix.size() - 1] == suffix.connectingLetter);
}

[[nodiscard]] bool suffixAttaches(const rel::Addon& suffix, const StemEntry& entry) {
  return !takesNoSuffix(entry.rules) && latin::suffixRootAdmits(suffix.root, entry.part) &&
         latin::stemKeyAdmits(entry.stemKey, suffix.rootStemKey, entry.part);
}

[[nodiscard]] bool prefixAttaches(const rel::Addon& prefix, const Part part, const std::uint16_t rules) {
  return latin::rootAdmits(prefix.root, part) && !takesNoPrefix(rules);
}

[[nodiscard]] bool rowFitsStem(const rel::ClassRowRecord& row, const StemEntry& entry) {
  return latin::stemKeyAdmits(entry.stemKey, rel::inflection(row.inflect).stemKey, entry.part);
}

[[nodiscard]] auto rowsWithEnding(const rel::Run rows, const Split& split) {
  const auto ids = std::views::iota(rows.first, rel::end(rows));
  const auto inflectOf = [](const std::uint32_t id) { return rel::classRow(id).inflect; };
  const auto first = std::ranges::lower_bound(ids, split.inflectionsOfEnding.first, {}, inflectOf);
  const auto last = std::ranges::lower_bound(first, ids.end(), rel::end(split.inflectionsOfEnding), {}, inflectOf);
  return std::ranges::subrange(first, last) | std::views::transform(rel::classRow);
}

[[nodiscard]] bool stemAllowed(const rel::ClassRowRecord& row, const StemEntry& entry, const std::uint16_t rules,
                               const std::string_view stem) {
  return latin::allowedStem(latin::AllowSet{rel::inflection(row.inflect).allow}, entry.verbKind,
                            isConjugationThreeOne(rules), stem);
}

[[nodiscard]] bool anyStemInDictionary(const Query& query, const std::span<const Split> splits) {
  for (const Split& split : splits)
    for (const StemEntry& entry : entriesWithStem(query, 0, split.stemLength))
      if (!isPackonOrQuPronoun(entry))
        return true;
  return false;
}

[[nodiscard]] bool anyNotAllowedPlainReading(const Query& query, const std::span<const Split> splits) {
  for (const Split& split : splits) {
    const std::string_view stem = query.folded.substr(0, split.stemLength);
    for (const StemEntry& entry : entriesWithStem(query, 0, split.stemLength)) {
      if (isPackonOrQuPronoun(entry))
        continue;
      for (const rel::ClassRowRecord& row : rowsWithEnding(entry.rows, split))
        if (rowFitsStem(row, entry) && !stemAllowed(row, entry, entry.rules, stem))
          return true;
    }
  }
  return false;
}

void addReading(const rel::ClassRowRecord& row, const StemEntry& entry, const std::uint16_t rules,
                const std::uint32_t orthOffsetInWord, const std::string_view orth, const std::string_view ending,
                const rel::Addon& addon, const rel::Addon* const secondAddon = nullptr) {
  const rel::InflectionRecord inflection = rel::inflection(row.inflect);
  ReadingToSweep found{};
  auto& candidate = found.candidate;
  rel::grammarReading(row.description, candidate.reading);
  candidate.reading.entry = entry.dictionary;
  candidate.reading.stem = static_cast<std::uint8_t>(entry.column | (orthOffsetInWord << 2));
  candidate.inflection = row.inflect;
  candidate.addon = addon.id;
  candidate.secondAddon = secondAddon ? secondAddon->id : parse::kNoAddon;
  candidate.orthLength = static_cast<std::uint8_t>(orth.size());
  found.entryAge = entry.age;
  found.entryFrequency = entry.frequency;
  found.allowed = stemAllowed(row, entry, rules, orth);
  (void)inflection;
  (void)ending;
  if (!affix().found.push(found))
    affix().outOfMemory = true;
}

void orderGroup(const std::size_t begin) {
  std::stable_sort(affix().found.begin() + static_cast<std::ptrdiff_t>(begin), affix().found.end(),
                   [](const ReadingToSweep& a, const ReadingToSweep& b) {
                     const auto& l = a.candidate;
                     const auto& r = b.candidate;
                     if (l.reading.entry != r.reading.entry)
                       return l.reading.entry < r.reading.entry;
                     return latin::listedBefore(
                         parse::grammar(l.reading),
                         std::char_traits<char>::length(rel::imageStrings() + rel::inflection(l.inflection).ending),
                         parse::grammar(r.reading),
                         std::char_traits<char>::length(rel::imageStrings() + rel::inflection(r.inflection).ending));
                   });
}

void dropNotAllowedAndRare(const std::size_t begin) {
  const auto first = affix().found.begin() + static_cast<std::ptrdiff_t>(begin);
  facts::RarityEvidence evidence{};
  for (const ReadingToSweep& found : std::ranges::subrange(first, affix().found.end())) {
    const auto inflection = rel::inflection(found.candidate.inflection);
    evidence.count(inflection.age, found.entryAge, inflection.frequency, found.entryFrequency);
  }
  const auto dropped = std::remove_if(first, affix().found.end(), [&](const ReadingToSweep& found) {
    const auto inflection = rel::inflection(found.candidate.inflection);
    return !found.allowed || evidence.drops(inflection.age, inflection.frequency);
  });
  affix().trimmed = affix().trimmed || dropped != affix().found.end();
  affix().found.eraseFrom(dropped);
}

void dropSameRecordAndGrammar(const std::size_t begin) {
  const auto repeats =
      std::ranges::unique(affix().found.begin() + static_cast<std::ptrdiff_t>(begin), affix().found.end(),
                          [](const ReadingToSweep& a, const ReadingToSweep& b) {
                            return a.candidate.reading.entry == b.candidate.reading.entry &&
                                   a.candidate.reading.part == b.candidate.reading.part &&
                                   a.candidate.reading.hasChange == b.candidate.reading.hasChange &&
                                   std::equal(a.candidate.reading.g, a.candidate.reading.g + 3, b.candidate.reading.g);
                          });
  affix().found.eraseFrom(repeats.begin());
}

void sweepNewGroup(const std::size_t begin) {
  orderGroup(begin);
  dropNotAllowedAndRare(begin);
  dropSameRecordAndGrammar(begin);
}

bool addPrefixReadings(const Query& query, const std::span<const Split> splits,
                       const rel::Addon* const suffix = nullptr) {
  const std::size_t suffixLength = suffix ? suffix->fix.size() : 0;

  const auto prefixes = rel::addonsOf(latin::AddonKind::prefix);
  // INFO: Only the marked prefixes, in table order.
  const unsigned first = prefixes.empty() ? 0 : prefixes.front().id;
  for (unsigned id = affix_walks::Marks::next(affix().marks.prefixes, first); id < first + prefixes.size();
       id = affix_walks::Marks::next(affix().marks.prefixes, id + 1)) {
    const rel::Addon& prefix = prefixes[id - first];
    if (!startsWithPrefix(query, prefix))
      continue;
    const std::size_t prefixLength = prefix.fix.size();
    const std::size_t before = affix().found.size();

    for (const Split& split : splits) {
      if (split.stemLength <= prefixLength)
        continue;
      const std::string_view stem = query.folded.substr(prefixLength, split.stemLength - prefixLength);
      const std::string_view orth = query.folded.substr(prefixLength, stem.size() + suffixLength);
      const std::string_view ending = query.folded.substr(prefixLength + orth.size());

      for (const StemEntry& entry :
           entriesWithStem(query, static_cast<std::uint32_t>(prefixLength), static_cast<std::uint32_t>(stem.size()))) {
        if (isPackonOrQuPronoun(entry))
          continue;
        if (suffix && !suffixAttaches(*suffix, entry))
          continue;
        const Part part = suffix ? suffix->targetPart : entry.part;
        const std::uint16_t rules = suffix ? suffix->targetRules : entry.rules;
        if (!prefixAttaches(prefix, part, rules))
          continue;

        for (const rel::ClassRowRecord& row : rowsWithEnding(suffix ? suffix->rows : entry.rows, split)) {
          // NOTE: A suffix's rows are keyed to it already.
          if (!suffix && !rowFitsStem(row, entry))
            continue;
          addReading(row, entry, rules, static_cast<std::uint32_t>(prefixLength), orth, ending, prefix, suffix);
        }
      }
    }

    const bool answered = affix().found.size() != before;
    sweepNewGroup(before);
    if (answered)
      return true;
  }
  return false;
}

[[nodiscard]] std::span<const Split> withoutSuffix(const Query& query, const std::span<const Split> splits,
                                                   const rel::Addon& suffix) {
  std::size_t count = 0;
  for (const Split& split : splits)
    if ((affix().marks.suffixCuts[suffix.id] & (std::uint32_t{1} << split.stemLength)) &&
        stemEndsWithSuffix(query, split.stemLength, suffix))
      affix().withoutSuffix[count++] =
          Split{split.stemLength - static_cast<std::uint32_t>(suffix.fix.size()), split.inflectionsOfEnding};
  return {affix().withoutSuffix.data(), count};
}

bool addSuffixReadings(const Query& query, const std::span<const Split> splits) {
  bool answered = false;
  const auto suffixes = rel::addonsOf(latin::AddonKind::suffix);
  // INFO: A suffix with no marked cut shortens no split.
  const unsigned first = suffixes.empty() ? 0 : suffixes.front().id;
  for (unsigned id = affix_walks::Marks::next(affix().marks.suffixes, first); id < first + suffixes.size();
       id = affix_walks::Marks::next(affix().marks.suffixes, id + 1)) {
    const rel::Addon& suffix = suffixes[id - first];
    const std::span<const Split> shortened = withoutSuffix(query, splits, suffix);
    if (shortened.empty())
      continue;

    if (!anyStemInDictionary(query, shortened)) {
      answered = addPrefixReadings(query, shortened, &suffix) || answered;
      continue;
    }

    const std::size_t before = affix().found.size();
    for (const Split& split : shortened) {
      const std::string_view orth = query.folded.substr(0, split.stemLength + suffix.fix.size());
      for (const StemEntry& entry : entriesWithStem(query, 0, split.stemLength)) {
        if (isPackonOrQuPronoun(entry) || !suffixAttaches(suffix, entry))
          continue;
        for (const rel::ClassRowRecord& row : rowsWithEnding(suffix.rows, split))
          addReading(row, entry, suffix.targetRules, 0, orth, query.folded.substr(orth.size()), suffix);
      }
    }
    answered = answered || affix().found.size() != before;
    sweepNewGroup(before);
  }
  return answered;
}

[[nodiscard]] std::span<const Split> splitAtEveryEnding(const std::string_view text) {
  std::size_t count = 0;
  for (std::size_t length = 0; length < text.size(); ++length) {
    const rel::Run inflections = rel::endingRun(text.substr(text.size() - length));
    if (!rel::empty(inflections))
      affix().splits[count++] = Split{static_cast<std::uint32_t>(text.size() - length), inflections};
  }
  return {affix().splits.data(), count};
}

// NOTE: No pronoun-kind check, as WORDS: quiquam is the INDEF packon on an ADJECT entry.
void addPackonReadings(const Query& query) {
  std::uint16_t number = 0;
  for (const rel::Addon& packon : rel::addonsOf(latin::AddonKind::packon)) {
    ++number;
    const std::string_view fix = packon.fix;
    if (fix.empty() || query.folded.size() <= fix.size() || !query.folded.ends_with(fix))
      continue;
    std::string_view base = query.folded.substr(0, query.folded.size() - fix.size());
    std::array<char, facts::kMaxWordCharacters> nBeforeDamAsM;
    if (fix == "dam" && base.ends_with('n')) {
      std::ranges::copy(base, nBeforeDamAsM.begin());
      nBeforeDamAsM[base.size() - 1] = 'm';
      base = {nBeforeDamAsM.data(), base.size()};
    }

    for (const Split& split : splitAtEveryEnding(base)) {
      const std::string_view stem = base.substr(0, split.stemLength);
      for (const StemEntry& entry : entriesWithStem(stem)) {
        if (entry.part != Part::packon || packonNumber(entry.rules) != number)
          continue;
        // NOTE: No stem-key check, as WORDS: cui.que is both DAT S X and NOM P M.
        for (const rel::ClassRowRecord& row : rowsWithEnding(entry.rows, split))
          addReading(row, entry, entry.rules, 0, stem, base.substr(split.stemLength), packon);
      }
    }
  }
}

template <typename Search> void collect(parse::Work& out, const std::string_view word, Search&& search) {
  affix().found.clear();
  affix().trimmed = false;
  affix().outOfMemory = false;
  if (word.empty() || word.size() > facts::kMaxWordCharacters)
    return;

  std::array<char, facts::kMaxWordCharacters> folded{};
  std::array<char, facts::kMaxWordCharacters> lowered{};
  for (std::size_t i = 0; i < word.size(); ++i) {
    folded[i] = facts::foldLetter(word[i]);
    lowered[i] = lower(word[i]);
  }
  search(Query{{folded.data(), word.size()}, {lowered.data(), word.size()}});

  out.trimmed |= affix().trimmed;
  out.outOfMemory |= affix().outOfMemory;
  for (const ReadingToSweep& found : affix().found) {
    const auto& c = found.candidate;
    parse::addAddonReading(out, word, c.reading, c.orthLength, c.inflection, c.addon, c.secondAddon);
  }
}

} // namespace

void packonReadings(parse::Work& out, const std::string_view word) {
  // NOTE: Every packon stem is cu or qu, and a packon word is 6 to 12 letters (a qu form of 3 to 6 and a fix of 3 to
  //  6); the return below relies on both.
  // TODO: Have gen derive the packon bounds.
  if (word.size() < 6 || word.size() > 12 || facts::foldLetter(word[1]) != 'u' ||
      (facts::foldLetter(word[0]) != 'q' && facts::foldLetter(word[0]) != 'c'))
    return;
  collect(out, word, [](const Query& query) {
    addPackonReadings(query);
    sweepNewGroup(0);
  });
}

void affixReadings(parse::Work& out, const std::string_view word) {
  collect(out, word, [](const Query& query) {
    affix().marks.begin(query.folded);
    std::size_t count = 0;
    affix().marks.endings(
        query.folded, [&](const unsigned cut, const rel::Run ending) { affix().splits[count++] = Split{cut, ending}; });
    const std::span<const Split> splits{affix().splits.data(), count};
    if (anyNotAllowedPlainReading(query, splits)) {
      affix().trimmed = true;
      return;
    }
    if (anyStemInDictionary(query, splits)) {
      if (!addSuffixReadings(query, splits))
        addPrefixReadings(query, splits);
    } else if (!addPrefixReadings(query, splits)) {
      addSuffixReadings(query, splits);
    }
  });
}

} // namespace whitaker
