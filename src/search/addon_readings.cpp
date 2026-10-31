#include "search/addon_readings.hpp"

#include "facts.hpp"
#include "search/relationship_image.hpp"
#include "search/relationship_schema.hpp"

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

[[nodiscard]] constexpr char lower(char value) noexcept {
  return value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a') : value;
}

struct Query {
  std::string_view folded;
  std::string_view lowered;
};

struct Split {
  std::uint32_t stemLength{};
  rel::Run inflectionsOfEnding;
};

struct ReadingToSweep {
  AddonReading reading;
  latin::Age entryAge{};
  latin::Frequency entryFrequency{};
  bool allowed{};
};

// NOTE: The library runs without exceptions, so a std::vector that cannot grow would end the caller's process. This
//  grows with realloc as newMatch does, and a failed growth is reported as out of memory.
template <typename T> class Grown {
  static_assert(std::is_trivially_copyable_v<T>);

public:
  Grown() noexcept = default;
  Grown(const Grown&) = delete;
  Grown& operator=(const Grown&) = delete;
  ~Grown() { std::free(m_items); }

  [[nodiscard]] bool push(const T& item) noexcept {
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
  void clear() noexcept { m_count = 0; }
  void eraseFrom(const T* first) noexcept { m_count = static_cast<std::size_t>(first - m_items); }
  [[nodiscard]] std::size_t size() const noexcept { return m_count; }
  [[nodiscard]] T* begin() noexcept { return m_items; }
  [[nodiscard]] T* end() noexcept { return m_items + m_count; }
  [[nodiscard]] std::span<const T> items() const noexcept { return {m_items, m_count}; }

private:
  T* m_items = nullptr;
  std::size_t m_count = 0;
  std::size_t m_capacity = 0;
};

thread_local Grown<ReadingToSweep> sFound;
thread_local Grown<AddonReading> sReturned;
// NOTE: At most one split per ending length, so the word's length bounds them.
thread_local std::array<Split, facts::kMaxWordCharacters> sSplits;
thread_local std::array<Split, facts::kMaxWordCharacters> sWithoutSuffix;
thread_local bool sTrimmed = false;
thread_local bool sOutOfMemory = false;

using rel::StemEntry;

[[nodiscard]] auto entriesWithStem(std::string_view stem) {
  const rel::Run stems = rel::stemRange(stem);
  return std::views::iota(stems.first, stems.end()) | std::views::transform(rel::stemEntry);
}

[[nodiscard]] bool isPackonOrQuPronoun(const StemEntry& entry) noexcept {
  return entry.part == Part::PACK || (entry.part == Part::PRON && rel::entry(entry.dictionary).which == 1);
}

[[nodiscard]] bool isConjugationThreeOne(std::uint16_t rules) noexcept { return (rules & rel::kRuleConjThreeOne) != 0; }

[[nodiscard]] std::uint16_t packonNumber(std::uint16_t rules) noexcept {
  return (rules >> rel::kRulePackonShift) & rel::kRulePackonMask;
}

[[nodiscard]] bool takesNoSuffix(std::uint16_t rules) noexcept { return (rules & rel::kRuleAbbreviation) != 0; }

[[nodiscard]] bool takesNoPrefix(std::uint16_t rules) noexcept {
  return (rules & (rel::kRuleAbbreviation | rel::kRuleInterjOrConj)) != 0;
}

// NOTE: The first letter as ADDONS spells it, so the capitalised II, V and X never fit a lowered word.
[[nodiscard]] bool startsWithPrefix(const Query& query, const rel::Addon& prefix) noexcept {
  const std::string_view fix = prefix.fix;
  return !fix.empty() && query.folded.size() > fix.size() && query.lowered.front() == prefix.firstLetterAsSpelled &&
         query.folded.starts_with(fix) &&
         (prefix.connectingLetter == '\0' || query.lowered[fix.size()] == prefix.connectingLetter);
}

[[nodiscard]] bool stemEndsWithSuffix(const Query& query, std::uint32_t stemLength, const rel::Addon& suffix) noexcept {
  const std::string_view fix = suffix.fix;
  const std::string_view stem = query.folded.substr(0, stemLength);
  return !fix.empty() && stem.size() > fix.size() && stem.ends_with(fix) &&
         (suffix.connectingLetter == '\0' || query.lowered[stem.size() - fix.size() - 1] == suffix.connectingLetter);
}

[[nodiscard]] bool suffixAttaches(const rel::Addon& suffix, const StemEntry& entry) noexcept {
  return !takesNoSuffix(entry.rules) && latin::suffixRootAdmits(suffix.root, entry.part) &&
         latin::stemKeyAdmits(entry.stemKey, suffix.rootStemKey, entry.part);
}

[[nodiscard]] bool prefixAttaches(const rel::Addon& prefix, Part part, std::uint16_t rules) noexcept {
  return latin::rootAdmits(prefix.root, part) && !takesNoPrefix(rules);
}

[[nodiscard]] bool rowFitsStem(const rel::ClassRowRecord& row, const StemEntry& entry) noexcept {
  return latin::stemKeyAdmits(entry.stemKey, rel::inflection(row.inflect).stemKey, entry.part);
}

[[nodiscard]] auto rowsWithEnding(rel::Run rows, const Split& split) {
  const auto ids = std::views::iota(rows.first, rows.end());
  const auto inflectOf = [](std::uint32_t id) { return rel::classRow(id).inflect; };
  const auto first = std::ranges::lower_bound(ids, split.inflectionsOfEnding.first, {}, inflectOf);
  const auto last = std::ranges::lower_bound(first, ids.end(), split.inflectionsOfEnding.end(), {}, inflectOf);
  return std::ranges::subrange(first, last) | std::views::transform(rel::classRow);
}

[[nodiscard]] bool stemAllowed(const rel::ClassRowRecord& row, const StemEntry& entry, std::uint16_t rules,
                               std::string_view stem) noexcept {
  return latin::allowedStem(latin::AllowSet{rel::inflection(row.inflect).allow}, entry.verbKind,
                            isConjugationThreeOne(rules), stem);
}

[[nodiscard]] bool anyStemInDictionary(const Query& query, std::span<const Split> splits) {
  for (const Split& split : splits)
    for (const StemEntry& entry : entriesWithStem(query.folded.substr(0, split.stemLength)))
      if (!isPackonOrQuPronoun(entry))
        return true;
  return false;
}

[[nodiscard]] bool anyNotAllowedPlainReading(const Query& query, std::span<const Split> splits) {
  for (const Split& split : splits) {
    const std::string_view stem = query.folded.substr(0, split.stemLength);
    for (const StemEntry& entry : entriesWithStem(stem)) {
      if (isPackonOrQuPronoun(entry))
        continue;
      for (const rel::ClassRowRecord& row : rowsWithEnding(entry.rows, split))
        if (rowFitsStem(row, entry) && !stemAllowed(row, entry, entry.rules, stem))
          return true;
    }
  }
  return false;
}

void addReading(const rel::ClassRowRecord& row, const StemEntry& entry, std::uint16_t rules,
                std::uint32_t orthOffsetInWord, std::string_view orth, std::string_view ending, const rel::Addon& addon,
                const rel::Addon* secondAddon = nullptr) {
  const rel::InflectionRecord inflection = rel::inflection(row.inflect);
  ReadingToSweep found{
      .reading = {.orthOffsetInWord = orthOffsetInWord,
                  .orthLength = static_cast<std::uint32_t>(orth.size()),
                  .grammar = rel::grammar(row.description),
                  .dictionary = entry.dictionary,
                  .addons = {AddonStep{addon.id, addon.kind},
                             secondAddon ? AddonStep{secondAddon->id, secondAddon->kind} : AddonStep{}},
                  .addonCount = static_cast<std::uint8_t>(secondAddon ? 2 : 1),
                  .inflectionAge = inflection.age,
                  .inflectionFrequency = inflection.frequency},
      .entryAge = entry.age,
      .entryFrequency = entry.frequency,
      .allowed = stemAllowed(row, entry, rules, orth)};
  std::ranges::copy(ending, found.reading.endingAsLookedUp.begin());
  found.reading.endingLength = static_cast<std::uint8_t>(ending.size());
  if (!sFound.push(found))
    sOutOfMemory = true;
}

void orderGroup(std::size_t begin) {
  std::stable_sort(sFound.begin() + static_cast<std::ptrdiff_t>(begin), sFound.end(),
                   [](const ReadingToSweep& a, const ReadingToSweep& b) {
                     const AddonReading& l = a.reading;
                     const AddonReading& r = b.reading;
                     if (l.dictionary != r.dictionary)
                       return l.dictionary < r.dictionary;
                     return latin::listedBefore(l.grammar, l.endingLength, r.grammar, r.endingLength);
                   });
}

void dropNotAllowedAndRare(std::size_t begin) {
  const auto first = sFound.begin() + static_cast<std::ptrdiff_t>(begin);
  facts::RarityEvidence evidence;
  for (const ReadingToSweep& found : std::ranges::subrange(first, sFound.end()))
    evidence.count(found.reading.inflectionAge, found.entryAge, found.reading.inflectionFrequency,
                   found.entryFrequency);
  const auto dropped = std::remove_if(first, sFound.end(), [&](const ReadingToSweep& found) {
    return !found.allowed || evidence.drops(found.reading.inflectionAge, found.reading.inflectionFrequency);
  });
  sTrimmed = sTrimmed || dropped != sFound.end();
  sFound.eraseFrom(dropped);
}

void dropSameRecordAndGrammar(std::size_t begin) {
  const auto repeats = std::ranges::unique(sFound.begin() + static_cast<std::ptrdiff_t>(begin), sFound.end(),
                                           [](const ReadingToSweep& a, const ReadingToSweep& b) {
                                             return a.reading.dictionary == b.reading.dictionary &&
                                                    a.reading.grammar == b.reading.grammar;
                                           });
  sFound.eraseFrom(repeats.begin());
}

void sweepNewGroup(std::size_t begin) {
  orderGroup(begin);
  dropNotAllowedAndRare(begin);
  dropSameRecordAndGrammar(begin);
}

bool addPrefixReadings(const Query& query, std::span<const Split> splits, const rel::Addon* suffix = nullptr) {
  const std::size_t suffixLength = suffix ? suffix->fix.size() : 0;

  for (const rel::Addon& prefix : rel::addonsOf(latin::AddonKind::Prefix)) {
    if (!startsWithPrefix(query, prefix))
      continue;
    const std::size_t prefixLength = prefix.fix.size();
    const std::size_t before = sFound.size();

    for (const Split& split : splits) {
      if (split.stemLength <= prefixLength)
        continue;
      const std::string_view stem = query.folded.substr(prefixLength, split.stemLength - prefixLength);
      const std::string_view orth = query.folded.substr(prefixLength, stem.size() + suffixLength);
      const std::string_view ending = query.folded.substr(prefixLength + orth.size());

      for (const StemEntry& entry : entriesWithStem(stem)) {
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

    const bool answered = sFound.size() != before;
    sweepNewGroup(before);
    if (answered)
      return true;
  }
  return false;
}

[[nodiscard]] std::span<const Split> withoutSuffix(const Query& query, std::span<const Split> splits,
                                                   const rel::Addon& suffix) {
  std::size_t count = 0;
  for (const Split& split : splits)
    if (stemEndsWithSuffix(query, split.stemLength, suffix))
      sWithoutSuffix[count++] =
          Split{split.stemLength - static_cast<std::uint32_t>(suffix.fix.size()), split.inflectionsOfEnding};
  return {sWithoutSuffix.data(), count};
}

bool addSuffixReadings(const Query& query, std::span<const Split> splits) {
  bool answered = false;
  for (const rel::Addon& suffix : rel::addonsOf(latin::AddonKind::Suffix)) {
    const std::span<const Split> shortened = withoutSuffix(query, splits, suffix);
    if (shortened.empty())
      continue;

    if (!anyStemInDictionary(query, shortened)) {
      answered = addPrefixReadings(query, shortened, &suffix) || answered;
      continue;
    }

    const std::size_t before = sFound.size();
    for (const Split& split : shortened) {
      const std::string_view orth = query.folded.substr(0, split.stemLength + suffix.fix.size());
      for (const StemEntry& entry : entriesWithStem(query.folded.substr(0, split.stemLength))) {
        if (isPackonOrQuPronoun(entry) || !suffixAttaches(suffix, entry))
          continue;
        for (const rel::ClassRowRecord& row : rowsWithEnding(suffix.rows, split))
          addReading(row, entry, suffix.targetRules, 0, orth, query.folded.substr(orth.size()), suffix);
      }
    }
    answered = answered || sFound.size() != before;
    sweepNewGroup(before);
  }
  return answered;
}

[[nodiscard]] std::span<const Split> splitAtEveryEnding(std::string_view text) {
  std::size_t count = 0;
  for (std::size_t length = 0; length < text.size(); ++length) {
    const rel::Run inflections = rel::endingRun(text.substr(text.size() - length));
    if (!inflections.empty())
      sSplits[count++] = Split{static_cast<std::uint32_t>(text.size() - length), inflections};
  }
  return {sSplits.data(), count};
}

// NOTE: No pronoun-kind check, as WORDS: quiquam is the INDEF packon on an ADJECT entry.
void addPackonReadings(const Query& query) {
  std::uint16_t number = 0;
  for (const rel::Addon& packon : rel::addonsOf(latin::AddonKind::Packon)) {
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
        if (entry.part != Part::PACK || packonNumber(entry.rules) != number)
          continue;
        // NOTE: No stem-key check, as WORDS: cui.que is both DAT S X and NOM P M.
        for (const rel::ClassRowRecord& row : rowsWithEnding(entry.rows, split))
          addReading(row, entry, entry.rules, 0, stem, base.substr(split.stemLength), packon);
      }
    }
  }
}

template <typename Search> [[nodiscard]] AddonReadings collect(std::string_view word, Search&& search) noexcept {
  sFound.clear();
  sReturned.clear();
  sTrimmed = false;
  sOutOfMemory = false;
  if (word.empty() || word.size() > facts::kMaxWordCharacters)
    return {};

  std::array<char, facts::kMaxWordCharacters> folded{};
  std::array<char, facts::kMaxWordCharacters> lowered{};
  for (std::size_t i = 0; i < word.size(); ++i) {
    folded[i] = facts::foldLetter(word[i]);
    lowered[i] = lower(word[i]);
  }
  search(Query{{folded.data(), word.size()}, {lowered.data(), word.size()}});

  for (const ReadingToSweep& found : sFound)
    if (!sReturned.push(found.reading))
      sOutOfMemory = true;
  return {sReturned.items(), sTrimmed, sOutOfMemory};
}

} // namespace

AddonReadings packonReadings(std::string_view word) noexcept {
  return collect(word, [](const Query& query) {
    addPackonReadings(query);
    sweepNewGroup(0);
  });
}

AddonReadings affixReadings(std::string_view word) noexcept {
  return collect(word, [](const Query& query) {
    const std::span<const Split> splits = splitAtEveryEnding(query.folded);
    if (anyNotAllowedPlainReading(query, splits)) {
      sTrimmed = true;
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
