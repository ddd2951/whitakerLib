#include "search/addon_fallback.hpp"

#include "facts.hpp"
#include "search/relationship_schema.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

namespace whitaker {
namespace {

namespace rel = whitaker::relationship;

using Image = rel::Image;
using Part = latin::Part;

// INFO: Lower_Case(Raw_Word). A prefix's first character and CONNECT are
//  compared against this, not against the folded query.
[[nodiscard]] constexpr char lower(char value) noexcept {
  return value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a')
                                      : value;
}

struct Query {
  std::string_view folded;
  std::string_view lowered;
};

// NOTE: Also an entry of Apply_Suffix's Ssa, where `stemLength` has the fix
//  taken off.
struct Split {
  std::uint32_t stemLength{};
  std::uint16_t inflectFirst{};
  std::uint16_t inflectCount{};
};

// NOTE: `allowed` is kept, not applied: a rejected reading still counts as
//  List_Sweep's evidence.
struct Hit {
  FallbackMatch match;
  latin::Age entryAge{};
  latin::Frequency entryFrequency{};
  latin::Age inflectAge{};
  latin::Frequency inflectFrequency{};
  bool allowed{};
};

thread_local std::vector<Hit> sHits;
thread_local std::vector<FallbackMatch> sMatches;
thread_local std::vector<Split> sSplits;
thread_local std::vector<Split> sSsa;
thread_local std::vector<bool> sFound;

[[nodiscard]] bool noFix(std::uint16_t gate) noexcept {
  return (gate & rel::kGateAbbreviation) != 0;
}

// INFO: A suffix replaces the part of speech, so only the prefix arm asks
//  about INTERJ and CONJ.
[[nodiscard]] bool noPrefix(std::uint16_t gate) noexcept {
  return (gate & (rel::kGateAbbreviation | rel::kGateInterjOrConj)) != 0;
}

// Which packon a PACK class answers to, or zero for any other class.
[[nodiscard]] std::uint16_t packonOf(std::uint16_t gate) noexcept {
  return (gate >> rel::kGatePackonShift) & rel::kGatePackonMask;
}

// INFO: Allowed_Stem. The inflection's share is in the image; the rest comes
//  off the entry and the stem the reading prints.
[[nodiscard]] bool allowedStem(latin::AllowSet allow, std::uint16_t gate,
                               latin::VerbKind kind,
                               std::string_view stem) noexcept {
  return latin::allowedStem(allow, kind, (gate & rel::kGateConjThreeOne) != 0,
                            stem);
}

struct Rows {
  std::uint32_t start{};
  std::uint16_t count{};
};

template <typename Visit>
void visitRows(const Image& image, Rows owner, const Split& split,
               Visit&& visit) {
  const std::uint16_t last =
      static_cast<std::uint16_t>(split.inflectFirst + split.inflectCount);
  std::uint32_t low = owner.start;
  std::uint32_t high = owner.start + owner.count;
  while (low < high) {
    const std::uint32_t middle = low + (high - low) / 2;
    if (image.fallbackRow(middle).inflect < split.inflectFirst)
      low = middle + 1;
    else
      high = middle;
  }
  for (std::uint32_t row = low; row < owner.start + owner.count; ++row) {
    const Image::FallbackRow entry = image.fallbackRow(row);
    if (entry.inflect >= last)
      break;
    visit(entry);
  }
}

struct Slot {
  std::uint16_t dictionary{};
  std::uint8_t key{};
  Part part{};
  std::uint16_t classId{};
  latin::VerbKind verbKind{};
  std::uint16_t gate{};
  latin::Age age{};
  latin::Frequency frequency{};
  Rows rows;
};

[[nodiscard]] Slot slotOf(const Image& image,
                          const Image::FallbackStem& stem) noexcept {
  const Image::DictionaryMetadata entry =
      image.dictionaryMetadata(stem.dictionary);
  const std::uint16_t id = entry.classId & rel::kClassIdMask;
  const Image::Class grammar = image.dictionaryClass(id);
  return Slot{.dictionary = stem.dictionary,
              .key = stem.key,
              .part = stem.part,
              .classId = id,
              .verbKind = latin::VerbKind{static_cast<std::uint8_t>(
                  (entry.classId >> rel::kClassVerbKindShift) &
                  rel::kClassVerbKindMask)},
              .gate = grammar.gate,
              .age = entry.age,
              .frequency = entry.frequency,
              .rows = Rows{grammar.rowStart, grammar.rowCount}};
}

void record(const Image& image, const Image::FallbackRow& row, const Slot& slot,
            std::uint16_t gate, std::uint32_t orthOffset,
            std::string_view shown, const Image::Addon& addon,
            const Image::Addon* secondAddon = nullptr) {
  const Image::Inflection inflection = image.inflection(row.inflect);
  sHits.push_back(Hit{
      .match = {.orthOffset = orthOffset,
                .orthLength = static_cast<std::uint32_t>(shown.size()),
                .meaning = image.dictionaryMeaning(slot.dictionary),
                .grammar = image.grammar(row.description),
                .dictionary = slot.dictionary,
                .addonId = {addon.id,
                            secondAddon ? secondAddon->id : std::uint16_t{}},
                .addonKind = {addon.kind, secondAddon ? secondAddon->kind
                                                      : latin::AddonKind{}},
                .addonCount = static_cast<std::uint8_t>(secondAddon ? 2 : 1)},
      .entryAge = slot.age,
      .entryFrequency = slot.frequency,
      .inflectAge = inflection.age,
      .inflectFrequency = inflection.frequency,
      .allowed = allowedStem(inflection.allow, gate, slot.verbKind, shown)});
}

// INFO: Apply_Prefix, word_package.adb:1140-1205. The first prefix that
//  answers ends the search.
// NOTE: `combined` is Apply_Suffix's else arm; the gates then read the
//  suffix's target, not the entry.
void applyPrefix(const Image& image, const Query& query,
                 std::span<const Split> splits, std::uint16_t suffix,
                 bool combined) {
  Image::Addon suffixRecord{};
  std::size_t suffixLength = 0;
  if (combined) {
    suffixRecord = image.addon(suffix);
    // NOTE: No target rows means no parse line, not match-anything.
    if (suffixRecord.rowCount == 0)
      return;
    suffixLength = std::string_view{suffixRecord.fix}.size();
  }

  for (std::uint16_t index = 0; index < image.addonCount(); ++index) {
    const Image::Addon addon = image.addon(index);
    if (addon.kind != latin::AddonKind::Prefix)
      continue;
    const std::string_view fix{addon.fix};
    if (fix.empty())
      continue;
    const std::size_t before = sHits.size();

    for (const Split& split : splits) {
      // INFO: First character compared unfolded; the query is lowercased,
      //  so the capitalised II, V and X never fire.
      if (split.stemLength == 0 || query.lowered.front() != addon.firstRaw)
        continue;
      if (split.stemLength <= fix.size())
        continue;
      if (query.folded.substr(0, fix.size()) != fix)
        continue;
      if (addon.connect != '\0' && query.lowered[fix.size()] != addon.connect)
        continue;

      const auto [first, last] = image.fallbackStemRange(
          query.folded.substr(fix.size(), split.stemLength - fix.size()));
      if (first == last)
        continue;

      // NOTE: The stem the reading prints: prefix off, suffix still on.
      const std::string_view shown = query.folded.substr(
          fix.size(), split.stemLength + suffixLength - fix.size());

      for (std::uint32_t s = first; s < last; ++s) {
        const Slot slot = slotOf(image, image.fallbackStem(s));
        // NOTE: A prefix alone never derives from a PACK half word. A marker,
        //  not a rule: ADDONS.LAT's wider X restriction is unmodelled.
        if (!combined && slot.part == Part::PACK)
          continue;

        // INFO: The suffix's gates read the entry: no fix on an
        //  abbreviation, then Root and Root_Key.
        if (combined) {
          if (noFix(slot.gate))
            continue;
          if (!latin::suffixRootAdmits(suffixRecord.root, slot.part))
            continue;
          if (!latin::stemKeyAdmits(slot.key, suffixRecord.rootKey, slot.part))
            continue;
        }

        const Part part = combined ? suffixRecord.targetPart : slot.part;
        const std::uint16_t gate =
            combined ? suffixRecord.targetGate : slot.gate;
        const Rows rows =
            combined ? Rows{suffixRecord.rowStart, suffixRecord.rowCount}
                     : slot.rows;

        // A prefix has no Root_Key; its gate is the part alone.
        if (!latin::rootAdmits(addon.root, part))
          continue;
        if (noPrefix(gate))
          continue;

        visitRows(image, rows, split, [&](const Image::FallbackRow& row) {
          // NOTE: A suffix's rows carry Target_Key; a class's rows do not,
          //  so the key is asked here.
          if (!combined &&
              !latin::stemKeyAdmits(slot.key, image.inflection(row.inflect).key,
                                    slot.part))
            return;
          record(image, row, slot, gate, static_cast<std::uint32_t>(fix.size()),
                 shown, addon, combined ? &suffixRecord : nullptr);
        });
      }
    }

    // INFO: Reduce_Stem_List tests the record, not one split.
    if (sHits.size() != before)
      return;
  }
}

// INFO: Apply_Suffix, word_package.adb:1207-1296. Every matching record
//  contributes.
// NOTE: Record outer, splits inner: "no dictionary hit" is a property of
//  the record.
void applySuffix(const Image& image, const Query& query,
                 std::span<const Split> splits) {
  for (std::uint16_t index = 0; index < image.addonCount(); ++index) {
    const Image::Addon addon = image.addon(index);
    if (addon.kind != latin::AddonKind::Suffix)
      continue;
    const std::string_view fix{addon.fix};
    if (fix.empty())
      continue;

    sSsa.clear();
    sFound.clear();
    bool anyDictionary = false;

    for (const Split& split : splits) {
      const std::string_view base = query.folded.substr(0, split.stemLength);
      if (fix.size() >= base.size())
        continue;
      if (base.substr(base.size() - fix.size()) != fix)
        continue;
      // INFO: Subtract_Suffix's CONNECT, compared unfolded.
      if (addon.connect != '\0' &&
          query.lowered[base.size() - fix.size() - 1] != addon.connect)
        continue;

      const std::uint32_t shortened =
          split.stemLength - static_cast<std::uint32_t>(fix.size());
      const auto [first, last] =
          image.fallbackStemRange(query.folded.substr(0, shortened));
      anyDictionary = anyDictionary || first != last;
      sSsa.push_back(Split{shortened, split.inflectFirst, split.inflectCount});
      sFound.push_back(first != last);
    }

    if (sSsa.empty())
      continue;

    // INFO: No dictionary hit at all: the shortened stems go to Apply_Prefix
    //  carrying this suffix.
    if (!anyDictionary) {
      applyPrefix(image, query, sSsa, index, true);
      continue;
    }

    if (addon.rowCount == 0)
      continue;

    for (std::size_t i = 0; i < sSsa.size(); ++i) {
      if (!sFound[i])
        continue;
      const Split& split = sSsa[i];
      // NOTE: A suffix reading prints the stem with the fix still on.
      const std::string_view shown =
          query.folded.substr(0, split.stemLength + fix.size());

      const auto [first, last] =
          image.fallbackStemRange(query.folded.substr(0, split.stemLength));
      for (std::uint32_t s = first; s < last; ++s) {
        const Slot slot = slotOf(image, image.fallbackStem(s));
        if (!latin::suffixRootAdmits(addon.root, slot.part))
          continue;
        if (!latin::stemKeyAdmits(slot.key, addon.rootKey, slot.part))
          continue;
        // NOTE: On the class the entry has, not the one the suffix gives it.
        if (noFix(slot.gate))
          continue;

        visitRows(image, Rows{addon.rowStart, addon.rowCount}, split,
                  [&](const Image::FallbackRow& row) {
                    record(image, row, slot, addon.targetGate, 0, shown, addon);
                  });
      }
    }
  }
}

// INFO: Try_Tackons' packon arm, word_package.adb:1862. Not a fallback: WORDS
//  runs it inside Word, and quocumque is an ADV and is reported here too.
// INFO: The pronoun kind is not joined: WORDS reports quiquam for the INDEF
//  tack against an ADJECT entry.
void applyPackon(const Image& image, const Query& query) {
  std::uint16_t ordinal = 0;
  for (std::uint16_t index = 0; index < image.addonCount(); ++index) {
    const Image::Addon addon = image.addon(index);
    if (addon.kind != latin::AddonKind::Packon)
      continue;
    ++ordinal;
    const std::string_view fix{addon.fix};
    if (fix.empty() || query.folded.size() <= fix.size())
      continue;
    if (query.folded.substr(query.folded.size() - fix.size()) != fix)
      continue;
    const std::string_view base =
        query.folded.substr(0, query.folded.size() - fix.size());

    for (std::size_t length = 0; length < base.size(); ++length) {
      const auto [first, count] =
          image.endingRun(base.substr(base.size() - length));
      if (count == 0)
        continue;
      const Split split{static_cast<std::uint32_t>(base.size() - length), first,
                        count};
      const std::string_view stem = base.substr(0, split.stemLength);

      const auto [begin, end] = image.fallbackStemRange(stem);
      for (std::uint32_t s = begin; s < end; ++s) {
        const Slot slot = slotOf(image, image.fallbackStem(s));
        if (slot.part != Part::PACK || packonOf(slot.gate) != ordinal)
          continue;
        visitRows(image, slot.rows, split, [&](const Image::FallbackRow& row) {
          // INFO: No stem-key gate: WORDS prints cui.que as DAT S X and
          //  NOM P M off different columns. Prints as PRON, base alone.
          record(image, row, slot, slot.gate, 0, stem, addon);
        });
      }
    }
  }
}

// INFO: List_Sweep's rarity pass. Its evidence is counted before Allowed_Stem
//  removes anything.
void trim() {
  bool notOnlyArchaic = false;
  bool notOnlyMedieval = false;
  bool notOnlyUncommon = false;
  for (const Hit& hit : sHits) {
    if (!latin::archaic(hit.inflectAge) && !latin::archaic(hit.entryAge))
      notOnlyArchaic = true;
    if (!latin::medieval(hit.inflectAge) && !latin::medieval(hit.entryAge))
      notOnlyMedieval = true;
    if (!latin::uncommonInflection(hit.inflectFrequency) &&
        !latin::uncommonEntry(hit.entryFrequency))
      notOnlyUncommon = true;
  }
  std::erase_if(sHits, [=](const Hit& hit) {
    return !hit.allowed || (notOnlyArchaic && latin::archaic(hit.inflectAge)) ||
           (notOnlyMedieval && latin::medieval(hit.inflectAge)) ||
           (notOnlyUncommon && latin::uncommonInflection(hit.inflectFrequency));
  });
}

template <typename Stages>
[[nodiscard]] std::span<const FallbackMatch> run(std::string_view word,
                                                 Stages&& stages) noexcept {
  sHits.clear();
  sMatches.clear();
  sSplits.clear();
  if (word.empty() || word.size() > facts::kMaxWordCharacters)
    return {};

  std::array<char, facts::kMaxWordCharacters> foldBuffer{};
  std::array<char, facts::kMaxWordCharacters> lowerBuffer{};
  for (std::size_t i = 0; i < word.size(); ++i) {
    foldBuffer[i] = facts::foldLetter(word[i]);
    lowerBuffer[i] = lower(word[i]);
  }
  const Query query{std::string_view{foldBuffer.data(), word.size()},
                    std::string_view{lowerBuffer.data(), word.size()}};

  stages(query);
  trim();

  sMatches.reserve(sHits.size());
  for (const Hit& hit : sHits)
    sMatches.push_back(hit.match);
  return sMatches;
}

} // namespace

std::span<const FallbackMatch> packonReadings(const Image& image,
                                              std::string_view word) noexcept {
  return run(word, [&image](const Query& query) { applyPackon(image, query); });
}

std::span<const FallbackMatch> addonFallback(const Image& image,
                                             std::string_view word) noexcept {
  return run(word, [&image](const Query& query) {
    // INFO: Run_Inflections, inverted: a stem is the query with an ending off,
    //  never empty.
    for (std::size_t length = 0; length < query.folded.size(); ++length) {
      const auto [first, count] =
          image.endingRun(query.folded.substr(query.folded.size() - length));
      if (count != 0)
        sSplits.push_back(
            Split{static_cast<std::uint32_t>(query.folded.size() - length),
                  first, count});
    }

    // INFO: Prune_Stems' precedence: prefixes first, suffixes only if nothing.
    applyPrefix(image, query, sSplits, 0, false);
    if (sHits.empty())
      applySuffix(image, query, sSplits);
  });
}

} // namespace whitaker
