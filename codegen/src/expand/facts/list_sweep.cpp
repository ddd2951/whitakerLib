#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "facts.hpp"
#include "expand/common/reading.hpp"
#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "expand/common/quality.hpp"
#include "expand/facts/list_sweep.hpp"
#include "word/word.hpp"

// INFO: Words_Engine.List_Sweep. Sweeping's fix blocks and Has_Noun_Abbreviation stay at runtime.

namespace {

using namespace expand;
using quality::inflectionsLess;

enum class DictionaryKind : unsigned char { General = 7, Unique = 10 };

struct ParseRecord {
  std::size_t at;
  DictionaryKind dK;
  std::uint32_t mnpc;
  std::string_view stem;
  latin::Analysis qual;
  latin::StemKey key;
  std::string_view ending;
  latin::Age age;
  latin::Frequency freq;
};

struct Dropped {
  std::size_t at;
  Fate fate;
};

[[nodiscard]] bool pronounOne(const latin::Analysis& qual) noexcept {
  return qual.part == latin::Part::PRON && qual.which == 1;
}

// INFO: Switch's "<" and Equ.
[[nodiscard]] bool switchLess(const latin::Analysis& left, const latin::Analysis& right) {
  if (pronounOne(left) && right.part == latin::Part::PRON)
    return left.variant.value < right.variant.value;
  return inflectionsLess(left, right);
}

[[nodiscard]] bool equ(const latin::Analysis& left, const latin::Analysis& right) {
  if (pronounOne(left) && right.part == latin::Part::PRON)
    return left.variant == right.variant;
  return left == right;
}

[[nodiscard]] bool sameRecord(const ParseRecord& a, const ParseRecord& b) {
  return a.dK == b.dK && a.mnpc == b.mnpc && a.stem == b.stem && a.qual == b.qual && a.key == b.key &&
         a.ending == b.ending && a.age == b.age && a.freq == b.freq;
}

// INFO: Order_Parse_Array's Compare.
[[nodiscard]] bool compare(const ParseRecord& l, const ParseRecord& r) {
  if (r.dK != l.dK)
    return r.dK > l.dK;
  if (r.mnpc != l.mnpc)
    return r.mnpc < l.mnpc;
  if (switchLess(r.qual, l.qual))
    return true;
  if (!equ(r.qual, l.qual))
    return false;
  if (r.ending.size() != l.ending.size())
    return r.ending.size() < l.ending.size();
  return inflectionsLess(r.qual, l.qual);
}

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

ParseRecord record(const Form& form, const Reading& reading, std::size_t at) {
  ParseRecord pr{
      .at = at,
      .dK = reading.unique ? DictionaryKind::Unique : DictionaryKind::General,
      .mnpc = std::visit([](const auto& origin) { return origin.entry.value; }, form.origin),
      .stem = reading.stem,
      .qual = emitter::analysisOf(reading.entry, reading.inflection, reading.key),
      .key = reading.key,
      .ending = std::visit(
          []<typename T>(const T& origin) -> std::string_view {
            if constexpr (requires { origin.inflection; })
              return word::entry(origin.inflection).ending;
            else
              return {};
          },
          form.origin),
      .age = reading.inflectionAge,
      .freq = reading.inflectionFrequency,
  };
  // INFO: Reset_Pronoun_Kind.
  if (const auto* entry = std::get_if<latin::Pronoun>(&reading.entry);
      entry != nullptr && entry->declension.value == 1 && pr.dK == DictionaryKind::General)
    pr.qual.variant.value = std::to_underlying(entry->pronounKind);
  return pr;
}

// INFO: Order_Parse_Array.
void orderParseArray(std::vector<ParseRecord>& sl, const std::vector<Form>& forms, const std::vector<Reading>& readings,
                     std::vector<Dropped>& dropped) {
  for (bool hits = true; hits;) {
    hits = false;
    for (std::size_t i = 0; i + 1 < sl.size(); ++i)
      if (compare(sl[i], sl[i + 1])) {
        std::swap(sl[i], sl[i + 1]);
        hits = true;
      }
  }

  bool notOnlyArchaic = false, notOnlyMedieval = false, notOnlyUncommon = false;
  for (const ParseRecord& pr : sl) {
    if (pr.dK != DictionaryKind::General)
      continue;
    const Reading& reading = readings[pr.at];
    if (!latin::archaic(pr.age) && !latin::archaic(reading.entryAge))
      notOnlyArchaic = true;
    if (!latin::medieval(pr.age) && !latin::medieval(reading.entryAge))
      notOnlyMedieval = true;
    if (!latin::uncommonInflection(pr.freq) && !latin::uncommonEntry(reading.entryFrequency))
      notOnlyUncommon = true;
  }

  for (std::size_t i = sl.size(); i-- > 0;) {
    const ParseRecord& pr = sl[i];
    Fate fate = Fate::kept;
    if (pr.dK == DictionaryKind::General && !allowedStem(readings[pr.at], forms[pr.at].spelling))
      fate = Fate::stemNotAllowed;
    else if (notOnlyArchaic && facts::kOmitArchaic && latin::archaic(pr.age))
      fate = Fate::onlyArchaic;
    else if (notOnlyMedieval && facts::kOmitMedieval && latin::medieval(pr.age))
      fate = Fate::onlyMedieval;
    else if (notOnlyUncommon && facts::kOmitUncommon && latin::uncommonInflection(pr.freq))
      fate = Fate::onlyUncommon;
    if (fate != Fate::kept) {
      dropped.push_back({pr.at, fate});
      sl.erase(sl.begin() + static_cast<std::ptrdiff_t>(i));
    }
  }
}

// INFO: List_Sweep's last chance to weed out duplicates.
void compress(std::vector<ParseRecord>& pa, std::vector<Dropped>& dropped) {
  if (pa.empty())
    return;
  ParseRecord opr = pa.front();
  std::size_t j = 1;
  const auto drop = [&](std::size_t at) {
    dropped.push_back({pa[at].at, Fate::duplicate});
    pa.erase(pa.begin() + static_cast<std::ptrdiff_t>(at));
  };
  while (j < pa.size()) {
    const ParseRecord pr = pa[j];
    if (!sameRecord(pr, opr)) {
      if (pr.qual == opr.qual && pr.mnpc == opr.mnpc)
        drop(j);
      else if (j + 1 < pa.size() && sameRecord(pa[j], pa[j + 1]))
        drop(j);
    } else {
      ++j;
    }
    opr = pr;
  }
}

void sweepGroup(std::vector<Form>& forms, std::vector<Reading>& readings, std::size_t begin, std::size_t end,
                std::vector<Fate>& out) {
  std::vector<ParseRecord> pa;
  pa.reserve(end - begin);
  for (std::size_t i = begin; i < end; ++i)
    pa.push_back(record(forms[i], readings[i], i));

  std::vector<Dropped> dropped;
  orderParseArray(pa, forms, readings, dropped);
  compress(pa, dropped);

  std::vector<Form> sweptForms;
  std::vector<Reading> sweptReadings;
  sweptForms.reserve(end - begin);
  sweptReadings.reserve(end - begin);
  for (const ParseRecord& pr : pa) {
    sweptForms.push_back(forms[pr.at]);
    sweptReadings.push_back(readings[pr.at]);
    out.push_back(Fate::kept);
  }
  for (const Dropped& d : dropped) {
    sweptForms.push_back(forms[d.at]);
    sweptReadings.push_back(readings[d.at]);
    out.push_back(d.fate);
  }
  std::ranges::copy(sweptForms, forms.begin() + static_cast<std::ptrdiff_t>(begin));
  std::ranges::copy(sweptReadings, readings.begin() + static_cast<std::ptrdiff_t>(begin));
}

} // namespace

expand::Swept expand::listSweep(Forms& listed, Readings& readings) {
  Swept swept;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    std::vector<Form>& forms = listed.byLetter[letter];
    std::vector<Reading>& read = readings.byLetter[letter];
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
