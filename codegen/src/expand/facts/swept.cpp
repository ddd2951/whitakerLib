#include <cstddef>
#include <string_view>
#include <variant>
#include <vector>

#include "facts.hpp"
#include "expand/common/reading.hpp"
#include "expand/facts/swept.hpp"

namespace {

using namespace expand;

bool allowedStem(const Reading& reading, std::string_view spelling) noexcept {
  const auto* inflected = std::get_if<latin::inflected::Verb>(&reading.inflection);
  if (inflected == nullptr)
    return true;
  const auto& verb = std::get<latin::Verb>(reading.entry);
  return latin::allowedStem(latin::allowOf(inflected->tense, inflected->voice, inflected->mood, inflected->person,
                                           inflected->number, spelling.size() == reading.stem.size()),
                            verb.verbKind, verb.conjugation.value == 3 && verb.conjugationVariant.value == 1,
                            reading.stem);
}

// INFO: Age and frequency filters apply only when another reading of this spelling avoids that condition.
void sweepGroup(const std::vector<Form>& forms, const std::vector<Reading>& readings, std::size_t begin,
                std::size_t end, std::vector<Fate>& out) {
  bool notOnlyArchaic = false, notOnlyMedieval = false, notOnlyUncommon = false;
  for (std::size_t i = begin; i < end; ++i) {
    const Reading& reading = readings[i];
    if (reading.unique)
      continue;
    if (!latin::archaic(reading.inflectionAge) && !latin::archaic(reading.entryAge))
      notOnlyArchaic = true;
    if (!latin::medieval(reading.inflectionAge) && !latin::medieval(reading.entryAge))
      notOnlyMedieval = true;
    if (!latin::uncommonInflection(reading.inflectionFrequency) && !latin::uncommonEntry(reading.entryFrequency))
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
    else if (notOnlyArchaic && facts::kOmitArchaic && latin::archaic(reading.inflectionAge))
      fate = Fate::onlyArchaic;
    else if (notOnlyMedieval && facts::kOmitMedieval && latin::medieval(reading.inflectionAge))
      fate = Fate::onlyMedieval;
    else if (notOnlyUncommon && facts::kOmitUncommon && latin::uncommonInflection(reading.inflectionFrequency))
      fate = Fate::onlyUncommon;
    out.push_back(fate);
  }
}

} // namespace

expand::Swept expand::swept(const Forms& listed, const Readings& readings) {
  Swept swept;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::vector<Form>& forms = listed.byLetter[letter];
    const std::vector<Reading>& read = readings.byLetter[letter];
    std::vector<Fate>& fates = swept.byLetter[letter];
    fates.reserve(forms.size());
    std::size_t begin = 0;
    while (begin < forms.size()) {
      std::size_t end = begin;
      while (end < forms.size() && spellingEqual(forms[end].spelling, forms[begin].spelling))
        ++end;
      sweepGroup(forms, read, begin, end, fates);
      begin = end;
    }
  }
  return swept;
}
