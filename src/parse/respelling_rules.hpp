#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace whitaker::parse {

// INFO: trick_tables.adb, in its order; `letter` is the first letter whose table holds the entry.
enum class SpellOp : std::uint8_t {
  replaceStart,
  swapStartFirstMatch,
  swapStartEitherWay,
  unassimilatePrefix,
  replaceAnywhere,
  startIsAsIis,
  endIsAsIis,
  doubleConsonant,
};

enum class SpellTable : std::uint8_t {
  prefixVariant,
  byFirstLetter,
  anywhere,
  medieval,
  ownSteps,
};

struct SpellEntry {
  SpellTable table{};
  char letter{};
  SpellOp op{};
  std::string_view first{}, second{};
};
static_assert(std::is_trivially_copyable_v<SpellEntry>);

inline constexpr std::array kSpellings{
    // INFO: Try_Slury: A, C, I, N, O, Q, S_Slur_Tricks.
    SpellEntry{SpellTable::prefixVariant, 'a', SpellOp::swapStartFirstMatch, "abs", "aps"},
    SpellEntry{SpellTable::prefixVariant, 'a', SpellOp::swapStartFirstMatch, "acq", "adq"},
    SpellEntry{SpellTable::prefixVariant, 'a', SpellOp::swapStartFirstMatch, "ante", "anti"},
    SpellEntry{SpellTable::prefixVariant, 'a', SpellOp::swapStartFirstMatch, "auri", "aure"},
    SpellEntry{SpellTable::prefixVariant, 'a', SpellOp::swapStartFirstMatch, "auri", "auru"},
    SpellEntry{SpellTable::prefixVariant, 'a', SpellOp::unassimilatePrefix, "ad", "a"},
    SpellEntry{SpellTable::prefixVariant, 'c', SpellOp::replaceStart, "circum", "circun"},
    SpellEntry{SpellTable::prefixVariant, 'c', SpellOp::swapStartFirstMatch, "con", "com"},
    SpellEntry{SpellTable::prefixVariant, 'c', SpellOp::replaceStart, "co", "com"},
    SpellEntry{SpellTable::prefixVariant, 'c', SpellOp::replaceStart, "co", "con"},
    SpellEntry{SpellTable::prefixVariant, 'c', SpellOp::swapStartFirstMatch, "conl", "coll"},
    SpellEntry{SpellTable::prefixVariant, 'i', SpellOp::unassimilatePrefix, "in", "i"},
    SpellEntry{SpellTable::prefixVariant, 'i', SpellOp::swapStartFirstMatch, "inb", "imb"},
    SpellEntry{SpellTable::prefixVariant, 'i', SpellOp::swapStartFirstMatch, "inp", "imp"},
    SpellEntry{SpellTable::prefixVariant, 'n', SpellOp::replaceStart, "nun", "non"},
    SpellEntry{SpellTable::prefixVariant, 'o', SpellOp::unassimilatePrefix, "ob", "o"},
    SpellEntry{SpellTable::prefixVariant, 'q', SpellOp::swapStartFirstMatch, "quadri", "quadru"},
    SpellEntry{SpellTable::prefixVariant, 's', SpellOp::replaceStart, "se", "ce"},
    SpellEntry{SpellTable::prefixVariant, 's', SpellOp::unassimilatePrefix, "sub", "su"},
    // INFO: Try_Tricks: A ... Z_Tricks.
    SpellEntry{SpellTable::byFirstLetter, 'a', SpellOp::swapStartEitherWay, "adgn", "agn"},
    SpellEntry{SpellTable::byFirstLetter, 'a', SpellOp::swapStartEitherWay, "adsc", "asc"},
    SpellEntry{SpellTable::byFirstLetter, 'a', SpellOp::swapStartEitherWay, "adsp", "asp"},
    SpellEntry{SpellTable::byFirstLetter, 'a', SpellOp::swapStartEitherWay, "arqui", "arci"},
    SpellEntry{SpellTable::byFirstLetter, 'a', SpellOp::swapStartEitherWay, "arqu", "arcu"},
    SpellEntry{SpellTable::byFirstLetter, 'a', SpellOp::replaceStart, "ae", "e"},
    SpellEntry{SpellTable::byFirstLetter, 'a', SpellOp::replaceStart, "al", "hal"},
    SpellEntry{SpellTable::byFirstLetter, 'a', SpellOp::replaceStart, "am", "ham"},
    SpellEntry{SpellTable::byFirstLetter, 'a', SpellOp::replaceStart, "ar", "har"},
    SpellEntry{SpellTable::byFirstLetter, 'a', SpellOp::replaceStart, "aur", "or"},
    SpellEntry{SpellTable::byFirstLetter, 'd', SpellOp::replaceStart, "dampn", "damn"},
    SpellEntry{SpellTable::byFirstLetter, 'd', SpellOp::swapStartEitherWay, "dij", "disj"},
    SpellEntry{SpellTable::byFirstLetter, 'd', SpellOp::swapStartEitherWay, "dir", "disr"},
    SpellEntry{SpellTable::byFirstLetter, 'd', SpellOp::swapStartEitherWay, "dir", "der"},
    SpellEntry{SpellTable::byFirstLetter, 'd', SpellOp::swapStartEitherWay, "del", "dil"},
    SpellEntry{SpellTable::byFirstLetter, 'e', SpellOp::swapStartEitherWay, "ecf", "eff"},
    SpellEntry{SpellTable::byFirstLetter, 'e', SpellOp::swapStartEitherWay, "ecs", "exs"},
    SpellEntry{SpellTable::byFirstLetter, 'e', SpellOp::swapStartEitherWay, "es", "ess"},
    SpellEntry{SpellTable::byFirstLetter, 'e', SpellOp::swapStartEitherWay, "ex", "exs"},
    SpellEntry{SpellTable::byFirstLetter, 'e', SpellOp::replaceStart, "eid", "id"},
    SpellEntry{SpellTable::byFirstLetter, 'e', SpellOp::replaceStart, "el", "hel"},
    SpellEntry{SpellTable::byFirstLetter, 'e', SpellOp::replaceStart, "e", "ae"},
    SpellEntry{SpellTable::byFirstLetter, 'f', SpellOp::swapStartEitherWay, "faen", "fen"},
    SpellEntry{SpellTable::byFirstLetter, 'f', SpellOp::swapStartEitherWay, "faen", "foen"},
    SpellEntry{SpellTable::byFirstLetter, 'f', SpellOp::swapStartEitherWay, "fed", "foed"},
    SpellEntry{SpellTable::byFirstLetter, 'f', SpellOp::swapStartEitherWay, "fet", "foet"},
    SpellEntry{SpellTable::byFirstLetter, 'f', SpellOp::replaceStart, "f", "ph"},
    SpellEntry{SpellTable::byFirstLetter, 'g', SpellOp::replaceStart, "gna", "na"},
    SpellEntry{SpellTable::byFirstLetter, 'h', SpellOp::replaceStart, "har", "ar"},
    SpellEntry{SpellTable::byFirstLetter, 'h', SpellOp::replaceStart, "hal", "al"},
    SpellEntry{SpellTable::byFirstLetter, 'h', SpellOp::replaceStart, "ham", "am"},
    SpellEntry{SpellTable::byFirstLetter, 'h', SpellOp::replaceStart, "hel", "el"},
    SpellEntry{SpellTable::byFirstLetter, 'h', SpellOp::replaceStart, "hol", "ol"},
    SpellEntry{SpellTable::byFirstLetter, 'h', SpellOp::replaceStart, "hum", "um"},
    SpellEntry{SpellTable::byFirstLetter, 'k', SpellOp::replaceStart, "k", "c"},
    SpellEntry{SpellTable::byFirstLetter, 'k', SpellOp::replaceStart, "c", "k"},
    SpellEntry{SpellTable::byFirstLetter, 'l', SpellOp::swapStartEitherWay, "lub", "lib"},
    SpellEntry{SpellTable::byFirstLetter, 'm', SpellOp::swapStartEitherWay, "mani", "manu"},
    SpellEntry{SpellTable::byFirstLetter, 'n', SpellOp::replaceStart, "na", "gna"},
    SpellEntry{SpellTable::byFirstLetter, 'n', SpellOp::swapStartEitherWay, "nihil", "nil"},
    SpellEntry{SpellTable::byFirstLetter, 'o', SpellOp::swapStartEitherWay, "obt", "opt"},
    SpellEntry{SpellTable::byFirstLetter, 'o', SpellOp::swapStartEitherWay, "obs", "ops"},
    SpellEntry{SpellTable::byFirstLetter, 'o', SpellOp::replaceStart, "ol", "hol"},
    SpellEntry{SpellTable::byFirstLetter, 'o', SpellOp::replaceStart, "opp", "op"},
    SpellEntry{SpellTable::byFirstLetter, 'o', SpellOp::replaceStart, "or", "aur"},
    SpellEntry{SpellTable::byFirstLetter, 'p', SpellOp::replaceStart, "ph", "f"},
    SpellEntry{SpellTable::byFirstLetter, 'p', SpellOp::swapStartEitherWay, "pre", "prae"},
    SpellEntry{SpellTable::byFirstLetter, 's', SpellOp::swapStartEitherWay, "subsc", "susc"},
    SpellEntry{SpellTable::byFirstLetter, 's', SpellOp::swapStartEitherWay, "subsp", "susp"},
    SpellEntry{SpellTable::byFirstLetter, 's', SpellOp::swapStartEitherWay, "subc", "susc"},
    SpellEntry{SpellTable::byFirstLetter, 's', SpellOp::swapStartEitherWay, "succ", "susc"},
    SpellEntry{SpellTable::byFirstLetter, 's', SpellOp::swapStartEitherWay, "subt", "supt"},
    SpellEntry{SpellTable::byFirstLetter, 's', SpellOp::swapStartEitherWay, "subt", "sust"},
    SpellEntry{SpellTable::byFirstLetter, 't', SpellOp::swapStartEitherWay, "transv", "trav"},
    SpellEntry{SpellTable::byFirstLetter, 'u', SpellOp::replaceStart, "ul", "hul"},
    SpellEntry{SpellTable::byFirstLetter, 'u', SpellOp::replaceStart, "uol", "vul"},
    SpellEntry{SpellTable::byFirstLetter, 'y', SpellOp::replaceStart, "y", "i"},
    SpellEntry{SpellTable::byFirstLetter, 'z', SpellOp::replaceStart, "z", "di"},
    // INFO: Any_Tricks.
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "ae", "e"},
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "bul", "bol"},
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "bol", "bul"},
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "cl", "cul"},
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "cu", "quu"},
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "f", "ph"},
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "ph", "f"},
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "h", ""},
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "oe", "e"},
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "vul", "vol"},
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "vol", "vul"},
    SpellEntry{SpellTable::anywhere, 0, SpellOp::replaceAnywhere, "uol", "vul"},
    // INFO: Mediaeval_Tricks.
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "col", "caul"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "e", "ae"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "o", "u"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "i", "y"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "ism", "sm"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "isp", "sp"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "ist", "st"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "iz", "z"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "esm", "sm"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "esp", "sp"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "est", "st"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "ez", "z"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "di", "z"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "f", "ph"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "is", "ix"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "b", "p"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "d", "t"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "v", "b"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "v", "f"},
    // NOTE: Twice in WORDS' table; the second never answers, but holds a rule id.
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "v", "f"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "s", "x"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "ci", "ti"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "nt", "nct"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "s", "ns"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "ch", "c"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "c", "ch"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "th", "t"},
    SpellEntry{SpellTable::medieval, 0, SpellOp::replaceAnywhere, "t", "th"},
    // INFO: Try_Tricks' own: is -> iis for eo, a terminal iis, a doubled consonant (Double_Consonants, by letter).
    SpellEntry{SpellTable::ownSteps, 'i', SpellOp::startIsAsIis, "is", "iis"},
    SpellEntry{SpellTable::ownSteps, 0, SpellOp::endIsAsIis, "iis", "is"},
    SpellEntry{SpellTable::ownSteps, 'b', SpellOp::doubleConsonant, "b", "bb"},
    SpellEntry{SpellTable::ownSteps, 'c', SpellOp::doubleConsonant, "c", "cc"},
    SpellEntry{SpellTable::ownSteps, 'd', SpellOp::doubleConsonant, "d", "dd"},
    SpellEntry{SpellTable::ownSteps, 'f', SpellOp::doubleConsonant, "f", "ff"},
    SpellEntry{SpellTable::ownSteps, 'g', SpellOp::doubleConsonant, "g", "gg"},
    SpellEntry{SpellTable::ownSteps, 'h', SpellOp::doubleConsonant, "h", "hh"},
    SpellEntry{SpellTable::ownSteps, 'j', SpellOp::doubleConsonant, "j", "jj"},
    SpellEntry{SpellTable::ownSteps, 'k', SpellOp::doubleConsonant, "k", "kk"},
    SpellEntry{SpellTable::ownSteps, 'l', SpellOp::doubleConsonant, "l", "ll"},
    SpellEntry{SpellTable::ownSteps, 'm', SpellOp::doubleConsonant, "m", "mm"},
    SpellEntry{SpellTable::ownSteps, 'n', SpellOp::doubleConsonant, "n", "nn"},
    SpellEntry{SpellTable::ownSteps, 'p', SpellOp::doubleConsonant, "p", "pp"},
    SpellEntry{SpellTable::ownSteps, 'q', SpellOp::doubleConsonant, "q", "qq"},
    SpellEntry{SpellTable::ownSteps, 'r', SpellOp::doubleConsonant, "r", "rr"},
    SpellEntry{SpellTable::ownSteps, 's', SpellOp::doubleConsonant, "s", "ss"},
    SpellEntry{SpellTable::ownSteps, 't', SpellOp::doubleConsonant, "t", "tt"},
    SpellEntry{SpellTable::ownSteps, 'v', SpellOp::doubleConsonant, "v", "vv"},
    SpellEntry{SpellTable::ownSteps, 'w', SpellOp::doubleConsonant, "w", "ww"},
    SpellEntry{SpellTable::ownSteps, 'x', SpellOp::doubleConsonant, "x", "xx"},
    SpellEntry{SpellTable::ownSteps, 'z', SpellOp::doubleConsonant, "z", "zz"},
};

// NOTE: UnassimilatePrefix keeps the id of the way WORDS never takes, so rule ids stay as published.
[[nodiscard]] constexpr bool twoWays(const SpellOp op) {
  return op == SpellOp::swapStartFirstMatch || op == SpellOp::unassimilatePrefix || op == SpellOp::swapStartEitherWay;
}
// INFO: A rule is an entry and a way: 1 + the count of ways before it, plus 1 when the word began with the second.
inline constexpr auto kFirstRule = [] {
  std::array<std::uint16_t, kSpellings.size() + 1> first{};
  for (std::size_t i = 0; i < kSpellings.size(); ++i)
    first[i + 1] = static_cast<std::uint16_t>(first[i] + (twoWays(kSpellings[i].op) ? 2 : 1));
  return first;
}();
static_assert(kFirstRule.back() < 256, "a rule id is one byte");
[[nodiscard]] constexpr std::uint8_t ruleOf(const std::size_t entry, const bool reversed) {
  return static_cast<std::uint8_t>(1 + kFirstRule[entry] + (reversed ? 1 : 0));
}

} // namespace whitaker::parse
