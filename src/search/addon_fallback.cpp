#include "search/addon_fallback.hpp"

#include "facts.hpp"
#include "search/relationship_schema.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>
#include <string_view>
#include <vector>

namespace whitaker {
namespace {

namespace rel = whitaker::relationship;

using Part = latin::Part;

// INFO: Lower_Case(Raw_Word). A prefix's first character and CONNECT are compared against this, not against the folded
//  query.
[[nodiscard]] constexpr char lower(char value) noexcept {
  return value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a') : value;
}

struct Query {
  std::string_view folded;
  std::string_view lowered;
};

// NOTE: Also an entry of Apply_Suffix's Ssa, where `stemLength` has the fix taken off.
struct StemAndEnding {
  std::uint32_t stemLength{};
  rel::Run inflects;
};

// NOTE: `allowed` is kept, not applied: a rejected reading still counts as List_Sweep's evidence.
struct FoundReading {
  FallbackMatch match;
  latin::Age entryAge{};
  latin::Frequency entryFrequency{};
  latin::Age inflectAge{};
  latin::Frequency inflectFrequency{};
  bool allowed{};
};

thread_local std::vector<FoundReading> sReadings;
thread_local std::vector<FallbackMatch> sMatches;
thread_local std::vector<StemAndEnding> sSplits;
thread_local std::vector<StemAndEnding> sShortened;

[[nodiscard]] bool takesNoAddon(std::uint16_t rules) noexcept { return (rules & rel::kRuleAbbreviation) != 0; }

// INFO: A suffix replaces the part of speech, so only the prefix arm asks about INTERJ and CONJ.
[[nodiscard]] bool takesNoPrefix(std::uint16_t rules) noexcept {
  return (rules & (rel::kRuleAbbreviation | rel::kRuleInterjOrConj)) != 0;
}

[[nodiscard]] std::uint16_t packonOf(std::uint16_t rules) noexcept {
  return (rules >> rel::kRulePackonShift) & rel::kRulePackonMask;
}

template <typename Visit> void visitRows(rel::Run rows, const StemAndEnding& split, Visit&& visit) {
  const auto rowIds = std::views::iota(rows.first, rows.end());
  const auto first = std::ranges::lower_bound(rowIds, split.inflects.first, {},
                                              [](std::uint32_t i) { return rel::fallbackRow(i).inflect; });
  for (const std::uint32_t row : std::ranges::subrange(first, rowIds.end())) {
    const rel::FallbackRowRecord entry = rel::fallbackRow(row);
    if (entry.inflect >= split.inflects.end())
      break;
    visit(entry);
  }
}

struct Candidate {
  std::uint16_t dictionary{};
  std::uint8_t stemKey{};
  Part part{};
  latin::VerbKind verbKind{};
  std::uint16_t rules{};
  latin::Age age{};
  latin::Frequency frequency{};
  rel::Run rows;
};

[[nodiscard]] Candidate candidateOf(const rel::FallbackStemRecord& stem) noexcept {
  const rel::DictionaryRecord entry = rel::dictionary(stem.dictionary);
  const std::uint16_t id = entry.classId & rel::kClassIdMask;
  const rel::ClassRecord grammar = rel::dictionaryClass(id);
  return Candidate{.dictionary = stem.dictionary,
                   .stemKey = stem.stemKey,
                   .part = stem.part,
                   .verbKind = latin::VerbKind{static_cast<std::uint8_t>((entry.classId >> rel::kClassVerbKindShift) &
                                                                         rel::kClassVerbKindMask)},
                   .rules = grammar.rules,
                   .age = entry.age,
                   .frequency = entry.frequency,
                   .rows = grammar.rows()};
}

void addReading(const rel::FallbackRowRecord& row, const Candidate& candidate, std::uint16_t rules,
                std::uint32_t orthOffset, std::string_view shown, const rel::Addon& addon,
                const rel::Addon* secondAddon = nullptr) {
  const rel::InflectionRecord inflection = rel::inflection(row.inflect);
  sReadings.push_back(
      FoundReading{.match = {.orthOffset = orthOffset,
                             .orthLength = static_cast<std::uint32_t>(shown.size()),
                             .grammar = rel::grammar(row.description),
                             .dictionary = candidate.dictionary,
                             .addonId = {addon.id, secondAddon ? secondAddon->id : std::uint16_t{}},
                             .addonKind = {addon.kind, secondAddon ? secondAddon->kind : latin::AddonKind{}},
                             .addonCount = static_cast<std::uint8_t>(secondAddon ? 2 : 1)},
                   .entryAge = candidate.age,
                   .entryFrequency = candidate.frequency,
                   .inflectAge = inflection.age,
                   .inflectFrequency = inflection.frequency,
                   .allowed = latin::allowedStem(latin::AllowSet{inflection.allow}, candidate.verbKind,
                                                 (rules & rel::kRuleConjThreeOne) != 0, shown)});
}

// INFO: Apply_Prefix, word_package.adb:1140-1205. The first prefix that
//  answers ends the search.
// NOTE: `afterSuffix` is Apply_Suffix's else arm; the rules then read the
//  suffix's target, not the entry.
void applyPrefix(const Query& query, std::span<const StemAndEnding> splits, std::uint16_t suffix, bool afterSuffix) {
  rel::Addon suffixRecord{};
  std::size_t suffixLength = 0;
  if (afterSuffix) {
    suffixRecord = rel::addon(suffix);
    // NOTE: No target rows means no parse line, not match-anything.
    if (suffixRecord.rows.empty())
      return;
    suffixLength = suffixRecord.fix.size();
  }

  for (const rel::Addon& addon : rel::addonsOf(latin::AddonKind::Prefix)) {
    const std::string_view fix = addon.fix;
    // INFO: First character compared unfolded; the query is lowercased, so the capitalised II, V and X never fire.
    if (fix.empty() || query.folded.size() <= fix.size() || query.lowered.front() != addon.firstLetterAsSpelled ||
        !query.folded.starts_with(fix) ||
        (addon.connectingLetter != '\0' && query.lowered[fix.size()] != addon.connectingLetter))
      continue;
    const std::size_t before = sReadings.size();

    for (const StemAndEnding& split : splits) {
      if (split.stemLength <= fix.size())
        continue;

      const rel::Run stems = rel::fallbackStemRange(query.folded.substr(fix.size(), split.stemLength - fix.size()));
      if (stems.empty())
        continue;

      // NOTE: The stem the reading prints: prefix off, suffix still on.
      const std::string_view shown = query.folded.substr(fix.size(), split.stemLength + suffixLength - fix.size());

      for (std::uint32_t s = stems.first; s < stems.end(); ++s) {
        const Candidate candidate = candidateOf(rel::fallbackStem(s));
        // NOTE: A prefix alone never derives from a PACK half word. A marker, not a rule: ADDONS.LAT's wider X
        //  restriction is unmodelled.
        if (!afterSuffix && candidate.part == Part::PACK)
          continue;

        // INFO: The suffix's gates read the entry: no fix on an abbreviation, then Root and Root_Key.
        if (afterSuffix) {
          if (takesNoAddon(candidate.rules))
            continue;
          if (!latin::suffixRootAdmits(suffixRecord.root, candidate.part))
            continue;
          if (!latin::stemKeyAdmits(candidate.stemKey, suffixRecord.rootStemKey, candidate.part))
            continue;
        }

        const Part part = afterSuffix ? suffixRecord.targetPart : candidate.part;
        const std::uint16_t rules = afterSuffix ? suffixRecord.targetRules : candidate.rules;
        const rel::Run rows = afterSuffix ? suffixRecord.rows : candidate.rows;

        // A prefix has no Root_Key; only its part is checked.
        if (!latin::rootAdmits(addon.root, part))
          continue;
        if (takesNoPrefix(rules))
          continue;

        visitRows(rows, split, [&](const rel::FallbackRowRecord& row) {
          // NOTE: A suffix's rows carry Target_Key; a class's rows do not, so the key is asked here.
          if (!afterSuffix &&
              !latin::stemKeyAdmits(candidate.stemKey, rel::inflection(row.inflect).stemKey, candidate.part))
            return;
          addReading(row, candidate, rules, static_cast<std::uint32_t>(fix.size()), shown, addon,
                     afterSuffix ? &suffixRecord : nullptr);
        });
      }
    }

    // INFO: Reduce_Stem_List tests the record, not one split.
    if (sReadings.size() != before)
      return;
  }
}

// INFO: Apply_Suffix, word_package.adb:1207-1296. Every matching record
//  contributes.
// NOTE: Record outer, splits inner: "no dictionary hit" is a property of
//  the record.
void applySuffix(const Query& query, std::span<const StemAndEnding> splits) {
  for (const rel::Addon& addon : rel::addonsOf(latin::AddonKind::Suffix)) {
    const std::string_view fix = addon.fix;
    if (fix.empty())
      continue;

    sShortened.clear();
    bool anyDictionary = false;

    for (const StemAndEnding& split : splits) {
      const std::string_view base = query.folded.substr(0, split.stemLength);
      if (fix.size() >= base.size())
        continue;
      if (!base.ends_with(fix))
        continue;
      // INFO: Subtract_Suffix's CONNECT, compared unfolded.
      if (addon.connectingLetter != '\0' && query.lowered[base.size() - fix.size() - 1] != addon.connectingLetter)
        continue;

      const std::uint32_t shortened = split.stemLength - static_cast<std::uint32_t>(fix.size());
      anyDictionary = anyDictionary || !rel::fallbackStemRange(query.folded.substr(0, shortened)).empty();
      sShortened.push_back(StemAndEnding{shortened, split.inflects});
    }

    if (sShortened.empty())
      continue;

    // INFO: No dictionary hit at all: the shortened stems go to Apply_Prefix carrying this suffix.
    if (!anyDictionary) {
      applyPrefix(query, sShortened, addon.id, true);
      continue;
    }

    if (addon.rows.empty())
      continue;

    for (const StemAndEnding& split : sShortened) {
      // NOTE: A suffix reading prints the stem with the fix still on.
      const std::string_view shown = query.folded.substr(0, split.stemLength + fix.size());

      const rel::Run stems = rel::fallbackStemRange(query.folded.substr(0, split.stemLength));
      for (std::uint32_t s = stems.first; s < stems.end(); ++s) {
        const Candidate candidate = candidateOf(rel::fallbackStem(s));
        if (!latin::suffixRootAdmits(addon.root, candidate.part))
          continue;
        if (!latin::stemKeyAdmits(candidate.stemKey, addon.rootStemKey, candidate.part))
          continue;
        // NOTE: On the class the entry has, not the one the suffix gives it.
        if (takesNoAddon(candidate.rules))
          continue;

        visitRows(addon.rows, split, [&](const rel::FallbackRowRecord& row) {
          addReading(row, candidate, addon.targetRules, 0, shown, addon);
        });
      }
    }
  }
}

// INFO: Run_Inflections, inverted.
void splitEndings(std::string_view text) {
  for (std::size_t length = 0; length < text.size(); ++length) {
    const rel::Run inflects = rel::endingRun(text.substr(text.size() - length));
    if (!inflects.empty())
      sSplits.push_back(StemAndEnding{static_cast<std::uint32_t>(text.size() - length), inflects});
  }
}

// INFO: Try_Tackons' packon arm, word_package.adb:1862. Not a fallback: WORDS
//  runs it inside Word, and quocumque is an ADV and is reported here too.
// INFO: The pronoun kind is not joined: WORDS reports quiquam for the INDEF
//  tack against an ADJECT entry.
void applyPackon(const Query& query) {
  std::uint16_t ordinal = 0;
  for (const rel::Addon& addon : rel::addonsOf(latin::AddonKind::Packon)) {
    ++ordinal;
    const std::string_view fix = addon.fix;
    if (fix.empty() || query.folded.size() <= fix.size())
      continue;
    if (!query.folded.ends_with(fix))
      continue;
    const std::string_view base = query.folded.substr(0, query.folded.size() - fix.size());

    sSplits.clear();
    splitEndings(base);
    for (const StemAndEnding& split : sSplits) {
      const std::string_view stem = base.substr(0, split.stemLength);

      const rel::Run stems = rel::fallbackStemRange(stem);
      for (std::uint32_t s = stems.first; s < stems.end(); ++s) {
        const Candidate candidate = candidateOf(rel::fallbackStem(s));
        if (candidate.part != Part::PACK || packonOf(candidate.rules) != ordinal)
          continue;
        visitRows(candidate.rows, split, [&](const rel::FallbackRowRecord& row) {
          // INFO: No stem-key check: WORDS prints cui.que as DAT S X and NOM P M off different columns. Prints as PRON,
          //  base alone.
          addReading(row, candidate, candidate.rules, 0, stem, addon);
        });
      }
    }
  }
}

// INFO: List_Sweep's rarity pass. Its evidence is counted before Allowed_Stem removes anything.
void dropRareReadings() {
  bool notOnlyArchaic = false;
  bool notOnlyMedieval = false;
  bool notOnlyUncommon = false;
  for (const FoundReading& reading : sReadings) {
    if (!latin::archaic(reading.inflectAge) && !latin::archaic(reading.entryAge))
      notOnlyArchaic = true;
    if (!latin::medieval(reading.inflectAge) && !latin::medieval(reading.entryAge))
      notOnlyMedieval = true;
    if (!latin::uncommonInflection(reading.inflectFrequency) && !latin::uncommonEntry(reading.entryFrequency))
      notOnlyUncommon = true;
  }
  std::erase_if(sReadings, [=](const FoundReading& reading) {
    return !reading.allowed || (notOnlyArchaic && latin::archaic(reading.inflectAge)) ||
           (notOnlyMedieval && latin::medieval(reading.inflectAge)) ||
           (notOnlyUncommon && latin::uncommonInflection(reading.inflectFrequency));
  });
}

template <typename Stages>
[[nodiscard]] std::span<const FallbackMatch> run(std::string_view word, Stages&& stages) noexcept {
  sReadings.clear();
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
  dropRareReadings();

  sMatches.reserve(sReadings.size());
  for (const FoundReading& reading : sReadings)
    sMatches.push_back(reading.match);
  return sMatches;
}

} // namespace

std::span<const FallbackMatch> packonReadings(std::string_view word) noexcept {
  return run(word, [](const Query& query) { applyPackon(query); });
}

std::span<const FallbackMatch> addonFallback(std::string_view word) noexcept {
  return run(word, [](const Query& query) {
    splitEndings(query.folded);

    // INFO: Prune_Stems' precedence: prefixes first, suffixes only if nothing.
    applyPrefix(query, sSplits, 0, false);
    if (sReadings.empty())
      applySuffix(query, sSplits);
  });
}

} // namespace whitaker
