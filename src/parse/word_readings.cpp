#include "parse/word_readings.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <utility>

namespace whitaker::parse {
namespace {

void addPackonReadings(Work& out, Text& text, std::string_view word) noexcept {
  addAddonReadings(out, text, word, whitaker::packonReadings(word));
}

[[nodiscard]] bool foldedEquals(std::string_view text, std::string_view fix) noexcept {
  return std::ranges::equal(text, fix, {}, facts::foldLetter);
}

[[nodiscard]] bool isDictionaryQuPronoun(const rel::Result& reading) noexcept {
  return reading.grammar.part == latin::Part::PRON && reading.grammar.which == 1 &&
         reading.dictionary < WHITAKER_GENERAL_RECORDS;
}

void addQuPronounAfterTickon(Work& out, Text& text, std::string_view word) noexcept {
  for (const rel::Addon& tickon : rel::addonsOf(latin::AddonKind::Tickon)) {
    const std::string_view fix = tickon.fix;
    if (fix.empty() || word.size() <= fix.size() || !foldedEquals(word.substr(0, fix.size()), fix))
      continue;

    const std::string_view base = word.substr(fix.size());
    if (base.size() < 3 || !(foldedEquals(base.substr(0, 2), "qu") || foldedEquals(base.substr(0, 2), "cu")))
      continue;

    const int before = out.count;
    addDictionaryReadings(out, text, base, isDictionaryQuPronoun);
    if (out.count == before)
      addPackonReadings(out, text, base);
    if (out.count != before) {
      prependAddon(out, before, tickon);
      return;
    }
  }
}

[[nodiscard]] bool declensionWithin(const WhitakerGrammar& g, const rel::Addon& tackon) noexcept {
  const auto which = tackon.target[0], variant = tackon.target[1];
  return (g.which == which && (g.variant == variant || variant == 0)) || (which == 0 && variant == 0 && g.which != 9);
}

[[nodiscard]] bool tackonAccepts(const Match& match, const rel::Addon& tackon) noexcept {
  const bool samePart = match.grammar.part == std::to_underlying(tackon.targetPart);
  switch (tackon.targetPart) {
  case latin::Part::X:
    return true;
  case latin::Part::ADJ:
    return samePart;
  case latin::Part::N:
  case latin::Part::PRON:
    return samePart && declensionWithin(match.grammar, tackon);
  default:
    return false;
  }
}

struct Labels {
  latin::Age age;
  latin::Age entryAge;
  latin::Frequency frequency;
  latin::Frequency entryFrequency;
};

[[nodiscard]] Labels labelsOf(const Match& m) noexcept {
  return {latin::Age{m.inflectionAge}, latin::Age{m.entry.age}, latin::Frequency{m.inflectionFrequency},
          latin::Frequency{m.entry.frequency}};
}

// NOTE: Returns whether it trimmed one.
bool sweepOnlyWhatTackonTakes(Work& out, Text& text, std::string_view base, int before,
                              const rel::Addon& tackon) noexcept {
  std::array<char, facts::kMaxWordCharacters> folded{};
  for (std::size_t i = 0; i < base.size(); ++i)
    folded[i] = facts::foldLetter(base[i]);
  const rel::Run run = rel::trimmedReadingRange({folded.data(), base.size()});
  if (run.empty())
    return false;
  const int kept = out.count;
  const Text::Checkpoint checkpoint = text.checkpoint();
  for (std::uint32_t r = run.first; r < run.end(); ++r) {
    const rel::TrimmedReadingRecord record = rel::trimmedReading(r);
    Match match;
    fillFromImage(match, text, base, rel::resultOf(record));
    if (!tackonAccepts(match, tackon))
      continue;
    Match* added = newMatch(out);
    if (!added)
      break;
    *added = match;
    added->whyTrimmed = record.kind;
  }
  facts::RarityEvidence evidence;
  for (int i = before; i < out.count; ++i) {
    if (out.matches[i].entry.id >= WHITAKER_GENERAL_RECORDS)
      continue;
    const Labels l = labelsOf(out.matches[i]);
    evidence.count(l.age, l.entryAge, l.frequency, l.entryFrequency);
  }
  bool trimmed = false;
  keepOnly(out, text, checkpoint, kept, [&](int i) {
    const Labels l = labelsOf(out.matches[i]);
    const bool drop = out.matches[i].whyTrimmed == rel::kTrimmedNotAllowed || evidence.drops(l.age, l.frequency);
    trimmed = trimmed || drop;
    return !drop;
  });
  if (out.count != kept)
    std::stable_sort(out.matches + before, out.matches + out.count, [](const Match& a, const Match& b) {
      if (a.entry.id != b.entry.id)
        return a.entry.id < b.entry.id;
      return latin::listedBefore(analysisOf(a.grammar), std::strlen(a.ending), analysisOf(b.grammar),
                                 std::strlen(b.ending));
    });
  return trimmed;
}

void addTackonReadings(Work& out, Text& text, std::string_view word, Stage stage) noexcept {
  for (const rel::Addon& tackon : rel::addonsOf(latin::AddonKind::Tackon).subspan(kEncliticTackons)) {
    const std::string_view base = withoutTackon(word, tackon);
    if (base.empty())
      continue;
    const int before = out.count;
    const std::uint8_t trimmed = out.trimmed;
    const Text::Checkpoint checkpoint = text.checkpoint();
    addWordReadings(out, text, base, stage);
    keepOnly(out, text, checkpoint, before, [&](int i) { return tackonAccepts(out.matches[i], tackon); });
    // NOTE: The image keeps trimmed readings for PRON and ADJ only.
    const bool sweeps =
        stage == Stage::Dictionary && (tackon.targetPart == latin::Part::PRON || tackon.targetPart == latin::Part::ADJ);
    const bool swept = sweeps && sweepOnlyWhatTackonTakes(out, text, base, before, tackon);
    // NOTE: Readings the tackon rejects are not trimmed: no "*" from them.
    if (out.count == before) {
      out.trimmed = trimmed;
      continue;
    }
    if (sweeps)
      out.trimmed = static_cast<std::uint8_t>(trimmed | (swept ? 1 : 0));
    prependAddon(out, before, tackon);
    return;
  }
}

} // namespace

[[nodiscard]] std::string_view withoutTackon(std::string_view word, const rel::Addon& tackon) noexcept {
  const std::string_view fix = tackon.fix;
  if (fix.empty() || word.size() <= fix.size() || !foldedEquals(word.substr(word.size() - fix.size()), fix))
    return {};
  return word.substr(0, word.size() - fix.size());
}

void addWordReadings(Work& out, Text& text, std::string_view word, Stage stage) noexcept {
  const int before = out.count;
  if (stage == Stage::Dictionary) {
    addDictionaryReadings(out, text, word);
    addQuPronounAfterTickon(out, text, word);
    addPackonReadings(out, text, word);
  } else {
    addAddonReadings(out, text, word, whitaker::affixReadings(word));
  }
  if (out.count == before)
    addTackonReadings(out, text, word, stage);
}

} // namespace whitaker::parse
