#include "parse/respellings.hpp"

#include "parse/find_readings.hpp"
#include "parse/respelling_rules.hpp"
#include "parse/word_readings.hpp"

#include <algorithm>
#include <array>
#include <utility>

namespace whitaker::parse {
namespace {

[[nodiscard]] constexpr bool isVowel(char c) noexcept {
  return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y';
}

[[nodiscard]] int tackonOf(const Match& m) noexcept {
  for (int a = 0; a < m.addonCount; ++a)
    if (m.addons[a].kind == WHITAKER_ADDON_TACKON)
      return m.addons[a].id;
  return -1;
}

// NOTE: Syncope too, which readings count, and what the answer must hold.
struct RespellingChecks {
  bool syncope = true;
  bool (*keep)(const Match&) = nullptr;
  bool (*accept)(const Work&) = nullptr;
};

[[nodiscard]] bool endsWithLoneTackon(const Work& out, int before) noexcept {
  const int last = tackonOf(out.matches[out.count - 1]);
  return last >= 0 && !(out.count - 1 > before && tackonOf(out.matches[out.count - 2]) == last);
}

bool tryRespelling(Work& out, Text& text, std::string_view rewritten, std::uint8_t rule, std::uint8_t route,
                   RespellingChecks checks = {}) noexcept {
  const int before = out.count;
  const std::uint8_t trimmed = out.trimmed;
  const Text::Checkpoint checkpoint = text.checkpoint();
  addWordReadings(out, text, rewritten, Stage::Dictionary);
  if (checks.syncope)
    addSyncopeReadings(out, text, rewritten);
  const bool foundOnlyTrimmed = out.count == before && out.trimmed != trimmed;
  if (foundOnlyTrimmed)
    return true;
  if (checks.keep)
    keepOnly(out, text, checkpoint, before, [&](int i) { return checks.keep(out.matches[i]); });
  const bool answered =
      out.count > before && !endsWithLoneTackon(out, before) && (!checks.accept || checks.accept(out));
  if (!answered) {
    out.count = before;
    out.trimmed = trimmed;
    text.rewind(checkpoint);
    return false;
  }
  for (int i = before; i < out.count; ++i) {
    out.matches[i].spellingRule = rule;
    if (out.matches[i].route == WHITAKER_ROUTE_DIRECT)
      out.matches[i].route = route;
  }
  return true;
}

[[nodiscard]] bool endsInEo(const Work& r) noexcept {
  const WhitakerGrammar& g = r.matches[r.count - 1].grammar;
  return g.part == WHITAKER_PART_V && g.which == 6 && g.variant == 1;
}

[[nodiscard]] bool adjectiveDatAblPlural(const Match& m) noexcept {
  const WhitakerGrammar& g = m.grammar;
  return g.part == WHITAKER_PART_ADJ && g.which == 1 && g.variant == 1 &&
         (g.case_of == WHITAKER_CASE_DAT || g.case_of == WHITAKER_CASE_ABL) && g.number == WHITAKER_NUMBER_P;
}

struct Respeller {
  std::array<char, facts::kMaxWordCharacters> lowered{};
  std::array<char, facts::kMaxWordCharacters + 8> buffer{};
  std::string_view word;
  explicit Respeller(std::string_view input) noexcept {
    for (std::size_t i = 0; i < input.size(); ++i)
      lowered[i] = input[i] >= 'A' && input[i] <= 'Z' ? static_cast<char>(input[i] - 'A' + 'a') : input[i];
    word = {lowered.data(), input.size()};
  }
  [[nodiscard]] std::string_view join(std::string_view a, std::string_view b, std::string_view c = {}) noexcept {
    if (a.size() + b.size() + c.size() > facts::kMaxWordCharacters)
      return {};
    auto at = std::ranges::copy(a, buffer.begin()).out;
    at = std::ranges::copy(b, at).out;
    std::ranges::copy(c, at);
    return {buffer.data(), a.size() + b.size() + c.size()};
  }
};

bool tryRule(Work& out, Text& text, Respeller& s, std::size_t index) noexcept {
  const SpellEntry& e = kSpellings[index];
  const std::string_view word = s.word, first = e.first, second = e.second;
  const std::uint8_t route = e.op == SpellOp::UnassimilatePrefix ? WHITAKER_ROUTE_SLUR : WHITAKER_ROUTE_WORD_MOD;
  const auto attempt = [&](std::string_view rewritten, bool reversed) {
    return !rewritten.empty() && tryRespelling(out, text, rewritten, ruleOf(index, reversed), route);
  };
  const auto starts = [&](std::string_view from) { return word.size() >= from.size() + 2 && word.starts_with(from); };
  const auto flip = [&](std::string_view from, std::string_view to, bool reversed) {
    return attempt(s.join(to, word.substr(from.size())), reversed);
  };
  switch (e.op) {
  case SpellOp::ReplaceStart:
    return starts(first) && flip(first, second, false);
  case SpellOp::SwapStartFirstMatch:
    if (starts(first))
      return flip(first, second, false);
    return starts(second) && flip(second, first, true);
  case SpellOp::SwapStartEitherWay:
    return (starts(first) && flip(first, second, false)) || (starts(second) && flip(second, first, true));
  case SpellOp::UnassimilatePrefix: {
    // NOTE: One way only: Slur's other arm never fires in WORDS (immitto is not read as inmitto).
    const std::size_t sl = first.size();
    if (word.size() < sl + 2 || !word.starts_with(first) || isVowel(word[sl]))
      return false;
    // NOTE: adtineo as attineo.
    const char doubled[1] = {word[sl]};
    return attempt(s.join(second, {doubled, 1}, word.substr(sl)), false);
  }
  case SpellOp::ReplaceAnywhere:
    for (std::size_t i = 0; i + first.size() <= word.size(); ++i)
      if (word.substr(i, first.size()) == first &&
          attempt(s.join(word.substr(0, i), second, word.substr(i + first.size())), false))
        return true;
    return false;
  default:
    return false;
  }
}

bool tryRules(Work& out, Text& text, Respeller& s, SpellTable table, char letter = '\0') noexcept {
  for (std::size_t index = 0; index < kSpellings.size(); ++index)
    if (kSpellings[index].table == table && (letter == '\0' || kSpellings[index].letter == letter) &&
        tryRule(out, text, s, index))
      return true;
  return false;
}

[[nodiscard]] WhitakerSpellingKind spellingKind(const SpellEntry& e) noexcept {
  switch (e.op) {
  case SpellOp::ReplaceStart:
    return e.table == SpellTable::PrefixVariant ? WHITAKER_SPELLING_SLURY_SWAP : WHITAKER_SPELLING_REPLACED;
  case SpellOp::SwapStartFirstMatch:
    return WHITAKER_SPELLING_SLURY_SWAP;
  case SpellOp::UnassimilatePrefix:
    return WHITAKER_SPELLING_SLUR;
  case SpellOp::SwapStartEitherWay:
    return WHITAKER_SPELLING_SWAP;
  case SpellOp::ReplaceAnywhere:
    return WHITAKER_SPELLING_INTERNAL;
  case SpellOp::StartIsAsIis:
    return WHITAKER_SPELLING_EO_IIS;
  case SpellOp::EndIsAsIis:
    return WHITAKER_SPELLING_TERMINAL_IIS;
  case SpellOp::DoubleConsonant:
    return WHITAKER_SPELLING_DOUBLED;
  }
  std::unreachable();
}

[[nodiscard]] std::size_t firstEntry(SpellOp op, char letter) noexcept {
  return static_cast<std::size_t>(
      std::ranges::find_if(kSpellings, [&](const SpellEntry& e) { return e.op == op && e.letter == letter; }) -
      kSpellings.begin());
}

bool tryStartIsAsIis(Work& out, Text& text, Respeller& s) noexcept {
  return s.word.starts_with("is") &&
         tryRespelling(out, text, s.join("i", s.word), ruleOf(firstEntry(SpellOp::StartIsAsIis, 'i'), false),
                       WHITAKER_ROUTE_WORD_MOD, {.accept = endsInEo});
}

bool tryEndIsAsIis(Work& out, Text& text, Respeller& s) noexcept {
  const std::string_view w = s.word;
  return w.size() > 3 && w.ends_with("is") &&
         tryRespelling(out, text, s.join(w.substr(0, w.size() - 2), "iis"),
                       ruleOf(firstEntry(SpellOp::EndIsAsIis, 0), false), WHITAKER_ROUTE_WORD_MOD,
                       {.syncope = false, .keep = adjectiveDatAblPlural});
}

bool tryDoubledConsonant(Work& out, Text& text, Respeller& s) noexcept {
  const std::string_view w = s.word;
  for (std::size_t i = 1; i + 1 < w.size(); ++i) {
    if (isVowel(w[i]) || !isVowel(w[i - 1]) || !isVowel(w[i + 1]))
      continue;
    const std::size_t entry = firstEntry(SpellOp::DoubleConsonant, w[i]);
    if (entry == kSpellings.size())
      continue;
    const char letter[1] = {w[i]};
    if (tryRespelling(out, text, s.join(w.substr(0, i + 1), {letter, 1}, w.substr(i + 1)), ruleOf(entry, false),
                      WHITAKER_ROUTE_WORD_MOD))
      return true;
  }
  return false;
}

} // namespace

bool addPrefixVariantReadings(Work& out, Text& text, std::string_view input) noexcept {
  Respeller s{input};
  return !s.word.empty() && tryRules(out, text, s, SpellTable::PrefixVariant, s.word.front());
}

// NOTE: WORDS' guesses, Two_Words and a bad Roman numeral, are not ported.
bool addRespelledReadings(Work& out, Text& text, std::string_view input) noexcept {
  Respeller s{input};
  if (s.word.empty())
    return false;
  if (s.word.front() == 'i' ? tryStartIsAsIis(out, text, s)
                            : tryRules(out, text, s, SpellTable::ByFirstLetter, s.word.front()))
    return true;
  if (tryRules(out, text, s, SpellTable::Anywhere) || tryEndIsAsIis(out, text, s))
    return true;
  return facts::kDoMedievalTricks &&
         (tryRules(out, text, s, SpellTable::Medieval) || tryDoubledConsonant(out, text, s));
}

void addRespelledEncliticReadings(Work& out, Text& text, std::string_view word) noexcept {
  for (const rel::Addon& tackon : rel::addonsOf(latin::AddonKind::Tackon).first(kEncliticTackons)) {
    const std::string_view base = withoutTackon(word, tackon);
    if (base.empty())
      continue;
    const int before = out.count;
    if (addRespelledReadings(out, text, base))
      prependAddon(out, before, tackon);
    return;
  }
}

WhitakerSpellingRule spellingRule(std::uint8_t rule) noexcept {
  for (std::size_t index = 0; index < kSpellings.size(); ++index)
    if (rule > kFirstRule[index] && rule <= kFirstRule[index + 1]) {
      const SpellEntry& e = kSpellings[index];
      return {e.first.data(), e.second.data(), static_cast<std::uint8_t>(rule - 1 - kFirstRule[index]),
              static_cast<std::uint8_t>(e.op == SpellOp::UnassimilatePrefix ? WHITAKER_ROUTE_SLUR
                                                                            : WHITAKER_ROUTE_WORD_MOD),
              static_cast<std::uint8_t>(spellingKind(e))};
    }
  return {};
}

} // namespace whitaker::parse
