#include "parse/word_readings.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cstring>
#include <utility>
#include <type_traits>

namespace whitaker::parse {
namespace {

void addPackonReadings(Work& out, const std::string_view word) { whitaker::packonReadings(out, word); }

[[nodiscard]] bool foldedEquals(const std::string_view text, const std::string_view fix) {
  return std::ranges::equal(text, fix, {}, facts::foldLetter);
}

[[nodiscard]] bool isDictionaryQuPronoun(const Reading& reading) {
  return part(reading) == latin::Part::PRON && (reading.g[0] >> 4) == 1 &&
         reading.entry < std::to_underlying(kFirstUnique);
}

void addQuPronounAfterTickon(Work& out, const std::string_view word) {
  // NOTE: Every tickon starts with e, n or s (ec, ne, nescio, neu, seu, si); the return below relies on it.
  // TODO: Have gen derive the tickons' first letters.
  if (word.empty())
    return;
  if (const char first = facts::foldLetter(word[0]); first != 'e' && first != 'n' && first != 's')
    return;
  for (const rel::Addon& tickon : rel::addonsOf(latin::AddonKind::tickon)) {
    const std::string_view fix = tickon.fix;
    if (fix.empty() || word.size() <= fix.size() || !foldedEquals(word.substr(0, fix.size()), fix))
      continue;

    const std::string_view base = word.substr(fix.size());
    if (base.size() < 3 || !(foldedEquals(base.substr(0, 2), "qu") || foldedEquals(base.substr(0, 2), "cu")))
      continue;

    const int before = out.count;
    addDictionaryReadings(out, base, isDictionaryQuPronoun);
    if (out.count == before)
      addPackonReadings(out, base);
    if (out.count != before) {
      prependAddon(out, before, tickon);
      return;
    }
  }
}

[[nodiscard]] bool declensionWithin(const latin::Analysis& g, const rel::Addon& tackon) {
  const auto which = tackon.target[0], variant = tackon.target[1];
  return (g.which.value == which && (g.variant.value == variant || variant == 0)) ||
         (which == 0 && variant == 0 && g.which.value != 9);
}

[[nodiscard]] bool tackonAccepts(const Match& match, const rel::Addon& tackon) {
  const bool samePart = part(match) == tackon.targetPart;
  switch (tackon.targetPart) {
    case latin::Part::X:
      return true;
    case latin::Part::ADJ:
      return samePart;
    case latin::Part::N:
    case latin::Part::PRON:
      return samePart && declensionWithin(grammar(match), tackon);
    case latin::Part::V:
    case latin::Part::ADV:
    case latin::Part::PREP:
    case latin::Part::NUM:
    case latin::Part::CONJ:
    case latin::Part::INTERJ:
    case latin::Part::PACK:
    case latin::Part::SUPINE:
    case latin::Part::VPAR:
    case latin::Part::NONE:
      break;
  }
  return false;
}

struct Labels {
  latin::Age age{};
  latin::Age entryAge{};
  latin::Frequency frequency{};
  latin::Frequency entryFrequency{};
};
static_assert(std::is_trivially_copyable_v<Labels>);

[[nodiscard]] Labels labelsOf(const Work& out, const Match& m) {
  const auto d = rel::dictionary(m.entry);
  return {age(out, m), d.age, frequency(out, m), d.frequency};
}

// NOTE: Returns whether it trimmed one.
bool sweepOnlyWhatTackonTakes(Work& out, const std::string_view base, const int before, const rel::Addon& tackon) {
  std::array<char, facts::kMaxWordCharacters> folded{};
  for (std::size_t i = 0; i < base.size(); ++i)
    folded[i] = facts::foldLetter(base[i]);
  const rel::Run run = rel::trimmedReadingRange({folded.data(), base.size()});
  if (rel::empty(run))
    return false;
  const int kept = out.count;
  std::array<bool, 256> notAllowed{};
  for (std::uint32_t r = run.first; r < rel::end(run); ++r) {
    const rel::TrimmedReadingRecord record = rel::trimmedReading(r);
    Match match;
    fillFromImage(out, match, base, rel::resultOf(record));
    if (!tackonAccepts(match, tackon))
      continue;
    Match* const added = newMatch(out);
    if (!added)
      break;
    *added = match;
    notAllowed[static_cast<unsigned>(out.count - kept - 1)] = record.kind == rel::kTrimmedNotAllowed;
  }
  facts::RarityEvidence evidence{};
  for (int i = before; i < out.count; ++i) {
    if (out.matches[i].entry >= std::to_underlying(kFirstUnique))
      continue;
    const Labels l = labelsOf(out, out.matches[i]);
    evidence.count(l.age, l.entryAge, l.frequency, l.entryFrequency);
  }
  bool trimmed = false;
  keepOnly(out, kept, [&](const int i) {
    const Labels l = labelsOf(out, out.matches[i]);
    const bool drop = notAllowed[static_cast<unsigned>(i - kept)] || evidence.drops(l.age, l.frequency);
    trimmed = trimmed || drop;
    return !drop;
  });
  if (out.count != kept)
    std::stable_sort(out.matches + before, out.matches + out.count, [&out](const Match& a, const Match& b) {
      if (a.entry != b.entry)
        return a.entry < b.entry;
      return latin::listedBefore(grammar(a), endingLength(out, a), grammar(b), endingLength(out, b));
    });
  return trimmed;
}

void addTackonReadings(Work& out, const std::string_view word, const Stage stage) {
  if (word.empty())
    return;
  // INFO: withoutTackon compares the folded word with the fix as stored, so only fixes ending in the folded last
  //  letter.
  const auto tackons = rel::addonsOf(latin::AddonKind::tackon);
  std::uint64_t todo = rel::kTackonsEndingIn[static_cast<unsigned char>(facts::foldLetter(word.back()))] &
                       ~((std::uint64_t{1} << kEncliticTackons) - 1);
  for (; todo; todo &= todo - 1) {
    const rel::Addon& tackon = tackons[static_cast<std::size_t>(std::countr_zero(todo))];
    const std::string_view base = withoutTackon(word, tackon);
    if (base.empty())
      continue;
    const int before = out.count;
    const std::uint8_t trimmed = out.trimmed;
    addWordReadings(out, base, stage);
    keepOnly(out, before, [&](const int i) { return tackonAccepts(out.matches[i], tackon); });
    // NOTE: The image keeps trimmed readings for PRON and ADJ only.
    const bool sweeps =
        stage == Stage::dictionary && (tackon.targetPart == latin::Part::PRON || tackon.targetPart == latin::Part::ADJ);
    const bool swept = sweeps && sweepOnlyWhatTackonTakes(out, base, before, tackon);
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

[[nodiscard]] std::string_view withoutTackon(const std::string_view word, const rel::Addon& tackon) {
  const std::string_view fix = tackon.fix;
  if (fix.empty() || word.size() <= fix.size() || !foldedEquals(word.substr(word.size() - fix.size()), fix))
    return {};
  return word.substr(0, word.size() - fix.size());
}

void addWordReadings(Work& out, const std::string_view word, const Stage stage) {
  const int before = out.count;
  if (stage == Stage::dictionary) {
    addDictionaryReadings(out, word);
    addQuPronounAfterTickon(out, word);
    addPackonReadings(out, word);
  } else {
    whitaker::affixReadings(out, word);
  }
  if (out.count == before)
    addTackonReadings(out, word, stage);
}

} // namespace whitaker::parse
