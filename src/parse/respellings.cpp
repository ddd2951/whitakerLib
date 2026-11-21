#include "parse/respellings.hpp"

#include "parse/find_readings.hpp"
#include "parse/respelling_rules.hpp"
#include "parse/word_readings.hpp"

#include <algorithm>
#include <array>
#include <utility>
#include <type_traits>

namespace whitaker::parse {
namespace {

[[nodiscard]] constexpr bool isVowel(const char c) {
  return c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u' || c == 'y';
}

[[nodiscard]] int tackonOf(const Work& out, const Match& m) {
  const Change& c = change(out, m);
  for (unsigned a = 0; a < c.addonCount; ++a)
    if (rel::addon(c.addons[a]).kind == latin::AddonKind::tackon)
      return c.addons[a];
  return -1;
}

// NOTE: Syncope too, which readings count, and what the answer must hold.
struct RespellingChecks {
  bool syncope = true;
  bool (*keep)(const Match&) = nullptr;
  bool (*accept)(const Work&) = nullptr;
};
static_assert(std::is_trivially_copyable_v<RespellingChecks>);

[[nodiscard]] bool endsWithLoneTackon(const Work& out, const int before) {
  const int last = tackonOf(out, out.matches[out.count - 1]);
  return last >= 0 && !(out.count - 1 > before && tackonOf(out, out.matches[out.count - 2]) == last);
}

bool tryRespelling(Work& out, const std::string_view rewritten, const std::uint8_t rule, const CRoute route,
                   const RespellingChecks checks = {}) {
  const int before = out.count;
  const std::uint8_t trimmed = out.trimmed;
  addWordReadings(out, rewritten, Stage::dictionary);
  if (checks.syncope)
    addSyncopeReadings(out, rewritten);
  const bool foundOnlyTrimmed = out.count == before && out.trimmed != trimmed;
  if (foundOnlyTrimmed)
    return true;
  if (checks.keep)
    keepOnly(out, before, [&](const int i) { return checks.keep(out.matches[i]); });
  const bool answered =
      out.count > before && !endsWithLoneTackon(out, before) && (!checks.accept || checks.accept(out));
  if (!answered) {
    out.count = before;
    out.trimmed = trimmed;
    return false;
  }
  for (int i = before; i < out.count; ++i) {
    Change c = change(out, out.matches[i]);
    c.spellingRule = rule;
    if (c.route == CRoute::direct)
      c.route = route;
    setChange(out, out.matches[i], c);
  }
  return true;
}

[[nodiscard]] bool endsInEo(const Work& r) {
  const auto g = grammar(r.matches[r.count - 1]);
  return g.part == latin::Part::verb && g.which.value == 6 && g.variant.value == 1;
}

[[nodiscard]] bool adjectiveDatAblPlural(const Match& m) {
  const auto g = grammar(m);
  return g.part == latin::Part::adjective && g.which.value == 1 && g.variant.value == 1 &&
         (g.caseOf == latin::Case::dative || g.caseOf == latin::Case::ablative) && g.number == latin::Number::plural;
}

struct Respeller {
  std::array<char, facts::kMaxWordCharacters> lowered{};
  std::array<char, facts::kMaxWordCharacters + 8> buffer{};
  std::string_view word{};
  explicit Respeller(const std::string_view input) {
    for (std::size_t i = 0; i < input.size(); ++i)
      lowered[i] = input[i] >= 'A' && input[i] <= 'Z' ? static_cast<char>(input[i] - 'A' + 'a') : input[i];
    word = {lowered.data(), input.size()};
  }
  [[nodiscard]] std::string_view join(const std::string_view a, const std::string_view b,
                                      const std::string_view c = {}) {
    if (a.size() + b.size() + c.size() > facts::kMaxWordCharacters)
      return {};
    auto at = std::ranges::copy(a, buffer.begin()).out;
    at = std::ranges::copy(b, at).out;
    std::ranges::copy(c, at);
    return {buffer.data(), a.size() + b.size() + c.size()};
  }
};

bool tryRule(Work& out, Respeller& s, const std::size_t index) {
  const SpellEntry& e = kSpellings[index];
  const std::string_view word = s.word, first = e.first, second = e.second;
  const CRoute route = e.op == SpellOp::unassimilatePrefix ? CRoute::slur : CRoute::wordMod;
  const auto attempt = [&](const std::string_view rewritten, const bool reversed) {
    return !rewritten.empty() && tryRespelling(out, rewritten, ruleOf(index, reversed), route);
  };
  const auto starts = [&](const std::string_view from) {
    return word.size() >= from.size() + 2 && word.starts_with(from);
  };
  const auto flip = [&](const std::string_view from, const std::string_view to, const bool reversed) {
    return attempt(s.join(to, word.substr(from.size())), reversed);
  };
  switch (e.op) {
    case SpellOp::replaceStart:
      return starts(first) && flip(first, second, false);
    case SpellOp::swapStartFirstMatch:
      if (starts(first))
        return flip(first, second, false);
      return starts(second) && flip(second, first, true);
    case SpellOp::swapStartEitherWay:
      return (starts(first) && flip(first, second, false)) || (starts(second) && flip(second, first, true));
    case SpellOp::unassimilatePrefix: {
      // NOTE: One way only: Slur's other arm never fires in WORDS (immitto is not read as inmitto).
      const std::size_t sl = first.size();
      if (word.size() < sl + 2 || !word.starts_with(first) || isVowel(word[sl]))
        return false;
      // NOTE: adtineo as attineo.
      const char doubled[1] = {word[sl]};
      return attempt(s.join(second, {doubled, 1}, word.substr(sl)), false);
    }
    case SpellOp::replaceAnywhere:
      for (std::size_t i = 0; i + first.size() <= word.size(); ++i)
        if (word.substr(i, first.size()) == first &&
            attempt(s.join(word.substr(0, i), second, word.substr(i + first.size())), false))
          return true;
      return false;
    case SpellOp::startIsAsIis:
    case SpellOp::endIsAsIis:
    case SpellOp::doubleConsonant:
      break;
  }
  return false;
}

// INFO: For each table and letter, the run of kSpellings holding its entries; letter 0 is the whole table.
struct RuleRun {
  std::uint16_t first{}, last{};
};
static_assert(std::is_trivially_copyable_v<RuleRun>);
inline constexpr auto kRuleRuns = [] {
  std::array<std::array<RuleRun, 256>, 5> runs{};
  for (std::size_t i = kSpellings.size(); i-- > 0;) {
    const auto& e = kSpellings[i];
    for (const unsigned letter : {0u, static_cast<unsigned>(static_cast<unsigned char>(e.letter))}) {
      auto& run = runs[static_cast<std::size_t>(e.table)][letter];
      if (run.last == 0)
        run.last = static_cast<std::uint16_t>(i + 1);
      run.first = static_cast<std::uint16_t>(i);
    }
  }
  return runs;
}();
// INFO: Adjacent, so a run holds only its own entries and a word visits no other (tryRules filters anyway).
static_assert([] {
  for (std::size_t table = 0; table < kRuleRuns.size(); ++table)
    for (std::size_t letter = 0; letter < kRuleRuns[table].size(); ++letter)
      for (std::size_t i = kRuleRuns[table][letter].first; i < kRuleRuns[table][letter].last; ++i)
        if (static_cast<std::size_t>(kSpellings[i].table) != table ||
            (letter != 0 && static_cast<unsigned char>(kSpellings[i].letter) != letter))
          return false;
  return true;
}());
[[nodiscard]] constexpr std::uint32_t lettersIn(const std::string_view text) {
  std::uint32_t letters = 0;
  for (const char c : text)
    if (c >= 'a' && c <= 'z')
      letters |= 1u << (c - 'a');
  return letters;
}
inline constexpr auto kRuleNeeds = [] {
  std::array<std::uint32_t, kSpellings.size()> needs{};
  for (std::size_t i = 0; i < kSpellings.size(); ++i)
    if (kSpellings[i].op == SpellOp::replaceAnywhere)
      needs[i] = lettersIn(kSpellings[i].first);
  return needs;
}();

bool tryRules(Work& out, Respeller& s, const SpellTable table, const char letter = '\0') {
  const RuleRun run = kRuleRuns[static_cast<std::size_t>(table)][static_cast<unsigned char>(letter)];
  const std::uint32_t letters = lettersIn(s.word);
  for (std::size_t index = run.first; index < run.last; ++index)
    if (kSpellings[index].table == table && (letter == '\0' || kSpellings[index].letter == letter) &&
        (kRuleNeeds[index] & ~letters) == 0 && tryRule(out, s, index))
      return true;
  return false;
}

[[nodiscard]] latin::SpellingKind spellingKind(const SpellEntry& e) {
  switch (e.op) {
    case SpellOp::replaceStart:
      return e.table == SpellTable::prefixVariant ? latin::SpellingKind::SLURY_SWAP : latin::SpellingKind::REPLACED;
    case SpellOp::swapStartFirstMatch:
      return latin::SpellingKind::SLURY_SWAP;
    case SpellOp::unassimilatePrefix:
      return latin::SpellingKind::SLUR;
    case SpellOp::swapStartEitherWay:
      return latin::SpellingKind::SWAP;
    case SpellOp::replaceAnywhere:
      return latin::SpellingKind::INTERNAL;
    case SpellOp::startIsAsIis:
      return latin::SpellingKind::EO_IIS;
    case SpellOp::endIsAsIis:
      return latin::SpellingKind::TERMINAL_IIS;
    case SpellOp::doubleConsonant:
      return latin::SpellingKind::DOUBLED;
  }
  std::unreachable();
}

[[nodiscard]] std::size_t firstEntry(const SpellOp op, const char letter) {
  return static_cast<std::size_t>(
      std::ranges::find_if(kSpellings, [&](const SpellEntry& e) { return e.op == op && e.letter == letter; }) -
      kSpellings.begin());
}

bool tryStartIsAsIis(Work& out, Respeller& s) {
  return s.word.starts_with("is") &&
         tryRespelling(out, s.join("i", s.word), ruleOf(firstEntry(SpellOp::startIsAsIis, 'i'), false), CRoute::wordMod,
                       {.accept = endsInEo});
}

bool tryEndIsAsIis(Work& out, Respeller& s) {
  const std::string_view w = s.word;
  return w.size() > 3 && w.ends_with("is") &&
         tryRespelling(out, s.join(w.substr(0, w.size() - 2), "iis"), ruleOf(firstEntry(SpellOp::endIsAsIis, 0), false),
                       CRoute::wordMod, {.syncope = false, .keep = adjectiveDatAblPlural});
}

bool tryDoubledConsonant(Work& out, Respeller& s) {
  const std::string_view w = s.word;
  for (std::size_t i = 1; i + 1 < w.size(); ++i) {
    if (isVowel(w[i]) || !isVowel(w[i - 1]) || !isVowel(w[i + 1]))
      continue;
    const std::size_t entry = firstEntry(SpellOp::doubleConsonant, w[i]);
    if (entry == kSpellings.size())
      continue;
    const char letter[1] = {w[i]};
    if (tryRespelling(out, s.join(w.substr(0, i + 1), {letter, 1}, w.substr(i + 1)), ruleOf(entry, false),
                      CRoute::wordMod))
      return true;
  }
  return false;
}

} // namespace

bool addPrefixVariantReadings(Work& out, const std::string_view input) {
  Respeller s{input};
  return !s.word.empty() && tryRules(out, s, SpellTable::prefixVariant, s.word.front());
}

// NOTE: WORDS' guesses, Two_Words and a bad Roman numeral, are not ported.
bool addRespelledReadings(Work& out, const std::string_view input) {
  Respeller s{input};
  if (s.word.empty())
    return false;
  if (s.word.front() == 'i' ? tryStartIsAsIis(out, s) : tryRules(out, s, SpellTable::byFirstLetter, s.word.front()))
    return true;
  if (tryRules(out, s, SpellTable::anywhere) || tryEndIsAsIis(out, s))
    return true;
  return facts::kDoMedievalTricks && (tryRules(out, s, SpellTable::medieval) || tryDoubledConsonant(out, s));
}

void addRespelledEncliticReadings(Work& out, const std::string_view word) {
  // NOTE: Called only once respelling the whole word has found nothing.
  WHITAKER_TRACE_HOOK(ran(trace::Pass::respelling, out));
  for (const rel::Addon& tackon : rel::addonsOf(latin::AddonKind::tackon).first(kEncliticTackons)) {
    const std::string_view base = withoutTackon(word, tackon);
    if (base.empty())
      continue;
    const int before = out.count;
    if (addRespelledReadings(out, base))
      prependAddon(out, before, tackon);
    return;
  }
}

SpellingRule spellingRule(const std::uint8_t rule) {
  for (std::size_t index = 0; index < kSpellings.size(); ++index)
    if (rule > kFirstRule[index] && rule <= kFirstRule[index + 1]) {
      const SpellEntry& e = kSpellings[index];
      return {e.first.data(), e.second.data(), rule - 1 - kFirstRule[index] != 0, e.op == SpellOp::unassimilatePrefix,
              spellingKind(e)};
    }
  return {};
}

} // namespace whitaker::parse
