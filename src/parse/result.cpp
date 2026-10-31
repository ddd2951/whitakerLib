#include "parse/result.hpp"

#include <cstdlib>
#include <limits>
#include <utility>

namespace whitaker::parse {
namespace {

[[nodiscard]] WhitakerEntry entryOf(std::uint16_t index) noexcept {
  const rel::Entry entry = rel::entry(index);
  const rel::DictionaryRecord labels = rel::dictionary(index);
  return {.part = std::to_underlying(entry.part),
          .which = entry.which,
          .variant = entry.variant,
          .gender = std::to_underlying(entry.gender),
          .kind = entry.kind,
          .comparison = std::to_underlying(entry.comparison),
          .numeral_sort = std::to_underlying(entry.numeralSort),
          .area = std::to_underlying(entry.area),
          .geography = std::to_underlying(entry.geography),
          .source = std::to_underlying(entry.source),
          .numeral_value = entry.numeralValue.value,
          .age = std::to_underlying(labels.age),
          .frequency = std::to_underlying(labels.frequency),
          .id = index};
}

[[nodiscard]] const char* meaningWithoutBar(std::uint16_t dictionary) noexcept {
  const char* meaning = rel::dictionaryMeaning(dictionary);
  while (*meaning == '|')
    ++meaning;
  while (*meaning == ' ')
    ++meaning;
  return meaning;
}

void fillEntry(Match& match, std::uint16_t dictionary, const latin::Analysis& grammar) noexcept {
  match.meaning = meaningWithoutBar(dictionary);
  match.pos = latin::name(grammar.part);
  match.grammar = grammarOf(grammar);
  match.entry = entryOf(dictionary);
}

[[nodiscard]] std::uint8_t addonKind(latin::AddonKind kind) noexcept {
  switch (kind) {
  case latin::AddonKind::Tickon:
    return WHITAKER_ADDON_TICKON;
  case latin::AddonKind::Prefix:
    return WHITAKER_ADDON_PREFIX;
  case latin::AddonKind::Suffix:
    return WHITAKER_ADDON_SUFFIX;
  case latin::AddonKind::Tackon:
    return WHITAKER_ADDON_TACKON;
  case latin::AddonKind::Packon:
    return WHITAKER_ADDON_PACKON;
  }
  std::unreachable();
}

void prependAddon(Match& match, std::uint16_t id, latin::AddonKind kind) noexcept {
  if (match.addonCount >= facts::kMaxAddonSteps)
    return;
  for (std::size_t i = match.addonCount; i > 0; --i)
    match.addons[i] = match.addons[i - 1];
  match.addons[0] = {id, addonKind(kind)};
  ++match.addonCount;
}

[[nodiscard]] bool isAdverbOfAdjective(const Match& match) noexcept {
  return match.grammar.part == WHITAKER_PART_ADV && match.entry.part == WHITAKER_PART_ADJ;
}

} // namespace

[[nodiscard]] WhitakerGrammar grammarOf(const latin::Analysis& a) noexcept {
  return {.part = std::to_underlying(a.part),
          .which = a.which,
          .variant = a.variant.value,
          .case_of = std::to_underlying(a.caseOf),
          .number = std::to_underlying(a.number),
          .gender = std::to_underlying(a.gender),
          .comparison = std::to_underlying(a.comparison),
          .numeral_sort = std::to_underlying(a.numeralSort),
          .tense = std::to_underlying(a.tense),
          .voice = std::to_underlying(a.voice),
          .mood = std::to_underlying(a.mood),
          .person = a.person.value};
}

[[nodiscard]] latin::Analysis analysisOf(const WhitakerGrammar& g) noexcept {
  return {.part = latin::Part{g.part},
          .which = g.which,
          .variant = {g.variant},
          .caseOf = latin::Case{g.case_of},
          .number = latin::Number{g.number},
          .gender = latin::Gender{g.gender},
          .comparison = latin::Comparison{g.comparison},
          .numeralSort = latin::NumeralSort{g.numeral_sort},
          .tense = latin::Tense{g.tense},
          .voice = latin::Voice{g.voice},
          .mood = latin::Mood{g.mood},
          .person = {g.person}};
}

Match* newMatch(Work& out) noexcept {
  if (out.count == out.capacity) {
    const int capacity = out.capacity == 0 ? 64 : out.capacity * 2;
    auto* const matches =
        static_cast<Match*>(std::realloc(out.matches, static_cast<std::size_t>(capacity) * sizeof(Match)));
    if (matches == nullptr) {
      out.outOfMemory = true;
      return nullptr;
    }
    out.matches = matches;
    out.capacity = capacity;
  }
  return &out.matches[out.count++];
}

void fillFromImage(Match& match, Text& text, std::string_view spelling, const rel::Result& reading) noexcept {
  match = {};
  char stem[facts::kMaxWordCharacters];
  match.orth = text.keep({stem, rel::printStem(spelling, reading.stemLength, reading.stemPattern, stem)});
  match.ending = text.keepFolded(spelling.substr(reading.stemLength));
  const auto [age, frequency] = rel::inflectionLabels(reading.description, match.ending);
  match.inflectionAge = std::to_underlying(age);
  match.inflectionFrequency = std::to_underlying(frequency);
  fillEntry(match, reading.dictionary, reading.grammar);
  if (isAdverbOfAdjective(match))
    match.route = WHITAKER_ROUTE_ADVERB;
}

void addDictionaryReadings(Work& out, Text& text, std::string_view spelling,
                           bool (*accept)(const rel::Result&)) noexcept {
  const rel::Program program = rel::lookup(spelling);
  if (program.trimmed)
    out.trimmed = 1;
  std::uint32_t cursor = program.begin;
  std::uint16_t lexeme = std::numeric_limits<std::uint16_t>::max();
  for (unsigned i = 0; i < program.count; ++i) {
    const rel::Result reading = rel::next(cursor, lexeme);
    if (accept && !accept(reading))
      continue;
    Match* added = newMatch(out);
    if (!added)
      return;
    fillFromImage(*added, text, spelling, reading);
  }
}

void prependAddon(Work& out, int first, const rel::Addon& addon) noexcept {
  for (int i = first; i < out.count; ++i)
    prependAddon(out.matches[i], addon.id, addon.kind);
}

void addAddonReadings(Work& out, Text& text, std::string_view word, const whitaker::AddonReadings& found) noexcept {
  if (found.trimmed)
    out.trimmed = 1;
  if (found.outOfMemory)
    out.outOfMemory = true;
  for (const whitaker::AddonReading& reading : found.readings) {
    Match* added = newMatch(out);
    if (!added)
      break;
    *added = {};
    added->orth = text.keep(word.substr(reading.orthOffsetInWord, reading.orthLength));
    added->ending = text.keep({reading.endingAsLookedUp.data(), reading.endingLength});
    added->inflectionAge = std::to_underlying(reading.inflectionAge);
    added->inflectionFrequency = std::to_underlying(reading.inflectionFrequency);
    fillEntry(*added, reading.dictionary, reading.grammar);
    for (std::uint8_t i = 0; i < reading.addonCount; ++i)
      added->addons[i] = {reading.addons[i].id, addonKind(reading.addons[i].kind)};
    added->addonCount = reading.addonCount;
  }
}

} // namespace whitaker::parse
