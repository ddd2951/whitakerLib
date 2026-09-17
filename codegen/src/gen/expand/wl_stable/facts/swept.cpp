#include <cstddef>
#include <map>
#include <print>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "facts.hpp"
#include "gen/expand/wl_stable/common/reading.hpp"
#include "gen/expand/wl_stable/facts/swept.hpp"
#include "util/reflect_util.hpp"

namespace {

using namespace wl_stable::expand;

constexpr bool archaic(TypeAge a) noexcept { return a == TypeAge::A; }
constexpr bool medieval(TypeAge a) noexcept {
  return a == TypeAge::F || a == TypeAge::G || a == TypeAge::H;
}
constexpr bool uncommonInflection(TypeFrequency f) noexcept {
  return f != TypeFrequency::X && f != TypeFrequency::NONE &&
         f != TypeFrequency::A && f != TypeFrequency::B;
}
constexpr bool uncommonEntry(TypeFrequency f) noexcept {
  return uncommonInflection(f) && f != TypeFrequency::C;
}

// INFO: Allowed_Stem's mood and tense ranges.
constexpr bool moodIndToInf(TypeMood m) noexcept {
  return m == TypeMood::IND || m == TypeMood::SUB || m == TypeMood::IMP ||
         m == TypeMood::INF;
}
constexpr bool moodIndToImp(TypeMood m) noexcept {
  return m == TypeMood::IND || m == TypeMood::SUB || m == TypeMood::IMP;
}
constexpr bool tensePresToFut(TypeTense t) noexcept {
  return t == TypeTense::PRES || t == TypeTense::IMPF || t == TypeTense::FUT;
}
constexpr bool tensePerfToFutp(TypeTense t) noexcept {
  return t == TypeTense::PERF || t == TypeTense::PLUP || t == TypeTense::FUTP;
}

// INFO: Allowed_Stem's verb filters.
bool allowedStem(const Reading& reading, std::string_view spelling) noexcept {
  const auto* inflected =
      std::get_if<tokenized::inflected::Verb>(&reading.inflection);
  if (inflected == nullptr)
    return true;
  const auto& verb = std::get<tokenized::Verb>(reading.entry);
  const std::string_view stem = reading.stem;
  bool allowed = true;

  // INFO: dic/duc/fac/fer shortened imperative (G&L 130.5).
  if (verb.conjugation.value == 3 && verb.conjugationVariant.value == 1 &&
      inflected->tense == TypeTense::PRES &&
      inflected->voice == TypeVoice::ACTIVE &&
      inflected->mood == TypeMood::IMP && inflected->person.value == 2 &&
      inflected->number == TypeNumber::S && spelling.size() == stem.size()) {
    const bool ok =
        stem.size() >= 3 && (stem.substr(stem.size() - 3) == "dic" ||
                             stem.substr(stem.size() - 3) == "duc" ||
                             stem.substr(stem.size() - 3) == "fac" ||
                             stem.substr(stem.size() - 3) == "fer");
    if (!ok)
      allowed = false;
  }

  if (inflected->mood == TypeMood::IMP &&
      !((inflected->tense == TypeTense::PRES && inflected->person.value == 2) ||
        (inflected->tense == TypeTense::FUT &&
         (inflected->person.value == 2 || inflected->person.value == 3))))
    allowed = false;

  if (verb.verbKind == TypeVerbKind::IMPERS && inflected->person.value != 3)
    allowed = false;

  // NOTE: The future active infinitive exception must be able to restore a
  // result.
  if (verb.verbKind == TypeVerbKind::DEP) {
    if (inflected->voice == TypeVoice::ACTIVE &&
        inflected->mood == TypeMood::INF && inflected->tense == TypeTense::FUT)
      allowed = true;
    else if (inflected->voice == TypeVoice::ACTIVE &&
             moodIndToInf(inflected->mood))
      allowed = false;
  }

  // INFO: Semi-deponents are deponent only in the perfect system.
  if (verb.verbKind == TypeVerbKind::SEMIDEP && moodIndToImp(inflected->mood) &&
      ((inflected->voice == TypeVoice::PASSIVE &&
        tensePresToFut(inflected->tense)) ||
       (inflected->voice == TypeVoice::ACTIVE &&
        tensePerfToFutp(inflected->tense))))
    allowed = false;

  return allowed;
}

// INFO: Rarity trimming is conditional: an uncommon reading is removed only
// when a more common reading exists for the same spelling.
void sweepGroup(const std::vector<Form>& forms,
                const std::vector<Reading>& readings, std::size_t begin,
                std::size_t end, std::vector<Fate>& out) {
  bool notOnlyArchaic = false, notOnlyMedieval = false, notOnlyUncommon = false;
  for (std::size_t i = begin; i < end; ++i) {
    const Reading& reading = readings[i];
    if (reading.unique)
      continue;
    if (!archaic(reading.inflectionAge) && !archaic(reading.entryAge))
      notOnlyArchaic = true;
    if (!medieval(reading.inflectionAge) && !medieval(reading.entryAge))
      notOnlyMedieval = true;
    if (!uncommonInflection(reading.inflectionFrequency) &&
        !uncommonEntry(reading.entryFrequency))
      notOnlyUncommon = true;
  }

  for (std::size_t i = begin; i < end; ++i) {
    const Reading& reading = readings[i];
    if (reading.unique) {
      out.push_back(Fate::kept);
      continue;
    }
    Fate fate = Fate::kept;
    if (!allowedStem(reading, forms[i].spelling))
      fate = Fate::stemNotAllowed;
    else if (notOnlyArchaic && facts::kOmitArchaic &&
             archaic(reading.inflectionAge))
      fate = Fate::onlyArchaic;
    else if (notOnlyMedieval && facts::kOmitMedieval &&
             medieval(reading.inflectionAge))
      fate = Fate::onlyMedieval;
    else if (notOnlyUncommon && facts::kOmitUncommon &&
             uncommonInflection(reading.inflectionFrequency))
      fate = Fate::onlyUncommon;
    out.push_back(fate);
  }
}

} // namespace

wl_stable::expand::Swept
wl_stable::expand::enrolled::swept(const Forms& listed,
                                   const Readings& readings) {
  Swept swept;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::vector<Form>& forms = listed.byLetter[letter];
    const std::vector<Reading>& read = readings.byLetter[letter];
    std::vector<Fate>& fates = swept.byLetter[letter];
    fates.reserve(forms.size());
    std::size_t begin = 0;
    while (begin < forms.size()) {
      std::size_t end = begin;
      while (end < forms.size() &&
             spellingEqual(forms[end].spelling, forms[begin].spelling))
        ++end;
      sweepGroup(forms, read, begin, end, fates);
      begin = end;
    }
  }
  return swept;
}

void wl_stable::expand::enrolled::report(const Swept& swept) {
  std::map<Fate, std::size_t> tally;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    std::size_t kept = 0;
    for (const Fate fate : swept.byLetter[letter]) {
      ++tally[fate];
      kept += fate == Fate::kept;
    }
    if (!swept.byLetter[letter].empty())
      std::println("kept for '{}': {}", facts::kLatinLetters[letter], kept);
  }
  std::string line;
  for (const auto& [fate, count] : tally)
    line += " " + std::string{util::enumToSv(fate)} + " " +
            std::to_string(count);
  std::println("fates:{}", line);
}
