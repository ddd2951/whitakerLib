#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace whitaker::parse {

// INFO: trick_tables.adb, in its order; `letter` is the first letter whose table holds the entry.
enum class SpellOp : std::uint8_t {
  ReplaceStart,
  SwapStartFirstMatch,
  SwapStartEitherWay,
  UnassimilatePrefix,
  ReplaceAnywhere,
  StartIsAsIis,
  EndIsAsIis,
  DoubleConsonant
};

enum class SpellTable : std::uint8_t { PrefixVariant, ByFirstLetter, Anywhere, Medieval, OwnSteps };

struct SpellEntry {
  SpellTable table;
  char letter;
  SpellOp op;
  std::string_view first, second;
};

inline constexpr std::array kSpellings{
    // Try_Slury: A, C, I, N, O, Q, S_Slur_Tricks.
    SpellEntry{SpellTable::PrefixVariant, 'a', SpellOp::SwapStartFirstMatch, "abs", "aps"},
    SpellEntry{SpellTable::PrefixVariant, 'a', SpellOp::SwapStartFirstMatch, "acq", "adq"},
    SpellEntry{SpellTable::PrefixVariant, 'a', SpellOp::SwapStartFirstMatch, "ante", "anti"},
    SpellEntry{SpellTable::PrefixVariant, 'a', SpellOp::SwapStartFirstMatch, "auri", "aure"},
    SpellEntry{SpellTable::PrefixVariant, 'a', SpellOp::SwapStartFirstMatch, "auri", "auru"},
    SpellEntry{SpellTable::PrefixVariant, 'a', SpellOp::UnassimilatePrefix, "ad", "a"},
    SpellEntry{SpellTable::PrefixVariant, 'c', SpellOp::ReplaceStart, "circum", "circun"},
    SpellEntry{SpellTable::PrefixVariant, 'c', SpellOp::SwapStartFirstMatch, "con", "com"},
    SpellEntry{SpellTable::PrefixVariant, 'c', SpellOp::ReplaceStart, "co", "com"},
    SpellEntry{SpellTable::PrefixVariant, 'c', SpellOp::ReplaceStart, "co", "con"},
    SpellEntry{SpellTable::PrefixVariant, 'c', SpellOp::SwapStartFirstMatch, "conl", "coll"},
    SpellEntry{SpellTable::PrefixVariant, 'i', SpellOp::UnassimilatePrefix, "in", "i"},
    SpellEntry{SpellTable::PrefixVariant, 'i', SpellOp::SwapStartFirstMatch, "inb", "imb"},
    SpellEntry{SpellTable::PrefixVariant, 'i', SpellOp::SwapStartFirstMatch, "inp", "imp"},
    SpellEntry{SpellTable::PrefixVariant, 'n', SpellOp::ReplaceStart, "nun", "non"},
    SpellEntry{SpellTable::PrefixVariant, 'o', SpellOp::UnassimilatePrefix, "ob", "o"},
    SpellEntry{SpellTable::PrefixVariant, 'q', SpellOp::SwapStartFirstMatch, "quadri", "quadru"},
    SpellEntry{SpellTable::PrefixVariant, 's', SpellOp::ReplaceStart, "se", "ce"},
    SpellEntry{SpellTable::PrefixVariant, 's', SpellOp::UnassimilatePrefix, "sub", "su"},
    // Try_Tricks: A ... Z_Tricks.
    SpellEntry{SpellTable::ByFirstLetter, 'a', SpellOp::SwapStartEitherWay, "adgn", "agn"},
    SpellEntry{SpellTable::ByFirstLetter, 'a', SpellOp::SwapStartEitherWay, "adsc", "asc"},
    SpellEntry{SpellTable::ByFirstLetter, 'a', SpellOp::SwapStartEitherWay, "adsp", "asp"},
    SpellEntry{SpellTable::ByFirstLetter, 'a', SpellOp::SwapStartEitherWay, "arqui", "arci"},
    SpellEntry{SpellTable::ByFirstLetter, 'a', SpellOp::SwapStartEitherWay, "arqu", "arcu"},
    SpellEntry{SpellTable::ByFirstLetter, 'a', SpellOp::ReplaceStart, "ae", "e"},
    SpellEntry{SpellTable::ByFirstLetter, 'a', SpellOp::ReplaceStart, "al", "hal"},
    SpellEntry{SpellTable::ByFirstLetter, 'a', SpellOp::ReplaceStart, "am", "ham"},
    SpellEntry{SpellTable::ByFirstLetter, 'a', SpellOp::ReplaceStart, "ar", "har"},
    SpellEntry{SpellTable::ByFirstLetter, 'a', SpellOp::ReplaceStart, "aur", "or"},
    SpellEntry{SpellTable::ByFirstLetter, 'd', SpellOp::ReplaceStart, "dampn", "damn"},
    SpellEntry{SpellTable::ByFirstLetter, 'd', SpellOp::SwapStartEitherWay, "dij", "disj"},
    SpellEntry{SpellTable::ByFirstLetter, 'd', SpellOp::SwapStartEitherWay, "dir", "disr"},
    SpellEntry{SpellTable::ByFirstLetter, 'd', SpellOp::SwapStartEitherWay, "dir", "der"},
    SpellEntry{SpellTable::ByFirstLetter, 'd', SpellOp::SwapStartEitherWay, "del", "dil"},
    SpellEntry{SpellTable::ByFirstLetter, 'e', SpellOp::SwapStartEitherWay, "ecf", "eff"},
    SpellEntry{SpellTable::ByFirstLetter, 'e', SpellOp::SwapStartEitherWay, "ecs", "exs"},
    SpellEntry{SpellTable::ByFirstLetter, 'e', SpellOp::SwapStartEitherWay, "es", "ess"},
    SpellEntry{SpellTable::ByFirstLetter, 'e', SpellOp::SwapStartEitherWay, "ex", "exs"},
    SpellEntry{SpellTable::ByFirstLetter, 'e', SpellOp::ReplaceStart, "eid", "id"},
    SpellEntry{SpellTable::ByFirstLetter, 'e', SpellOp::ReplaceStart, "el", "hel"},
    SpellEntry{SpellTable::ByFirstLetter, 'e', SpellOp::ReplaceStart, "e", "ae"},
    SpellEntry{SpellTable::ByFirstLetter, 'f', SpellOp::SwapStartEitherWay, "faen", "fen"},
    SpellEntry{SpellTable::ByFirstLetter, 'f', SpellOp::SwapStartEitherWay, "faen", "foen"},
    SpellEntry{SpellTable::ByFirstLetter, 'f', SpellOp::SwapStartEitherWay, "fed", "foed"},
    SpellEntry{SpellTable::ByFirstLetter, 'f', SpellOp::SwapStartEitherWay, "fet", "foet"},
    SpellEntry{SpellTable::ByFirstLetter, 'f', SpellOp::ReplaceStart, "f", "ph"},
    SpellEntry{SpellTable::ByFirstLetter, 'g', SpellOp::ReplaceStart, "gna", "na"},
    SpellEntry{SpellTable::ByFirstLetter, 'h', SpellOp::ReplaceStart, "har", "ar"},
    SpellEntry{SpellTable::ByFirstLetter, 'h', SpellOp::ReplaceStart, "hal", "al"},
    SpellEntry{SpellTable::ByFirstLetter, 'h', SpellOp::ReplaceStart, "ham", "am"},
    SpellEntry{SpellTable::ByFirstLetter, 'h', SpellOp::ReplaceStart, "hel", "el"},
    SpellEntry{SpellTable::ByFirstLetter, 'h', SpellOp::ReplaceStart, "hol", "ol"},
    SpellEntry{SpellTable::ByFirstLetter, 'h', SpellOp::ReplaceStart, "hum", "um"},
    SpellEntry{SpellTable::ByFirstLetter, 'k', SpellOp::ReplaceStart, "k", "c"},
    SpellEntry{SpellTable::ByFirstLetter, 'k', SpellOp::ReplaceStart, "c", "k"},
    SpellEntry{SpellTable::ByFirstLetter, 'l', SpellOp::SwapStartEitherWay, "lub", "lib"},
    SpellEntry{SpellTable::ByFirstLetter, 'm', SpellOp::SwapStartEitherWay, "mani", "manu"},
    SpellEntry{SpellTable::ByFirstLetter, 'n', SpellOp::ReplaceStart, "na", "gna"},
    SpellEntry{SpellTable::ByFirstLetter, 'n', SpellOp::SwapStartEitherWay, "nihil", "nil"},
    SpellEntry{SpellTable::ByFirstLetter, 'o', SpellOp::SwapStartEitherWay, "obt", "opt"},
    SpellEntry{SpellTable::ByFirstLetter, 'o', SpellOp::SwapStartEitherWay, "obs", "ops"},
    SpellEntry{SpellTable::ByFirstLetter, 'o', SpellOp::ReplaceStart, "ol", "hol"},
    SpellEntry{SpellTable::ByFirstLetter, 'o', SpellOp::ReplaceStart, "opp", "op"},
    SpellEntry{SpellTable::ByFirstLetter, 'o', SpellOp::ReplaceStart, "or", "aur"},
    SpellEntry{SpellTable::ByFirstLetter, 'p', SpellOp::ReplaceStart, "ph", "f"},
    SpellEntry{SpellTable::ByFirstLetter, 'p', SpellOp::SwapStartEitherWay, "pre", "prae"},
    SpellEntry{SpellTable::ByFirstLetter, 's', SpellOp::SwapStartEitherWay, "subsc", "susc"},
    SpellEntry{SpellTable::ByFirstLetter, 's', SpellOp::SwapStartEitherWay, "subsp", "susp"},
    SpellEntry{SpellTable::ByFirstLetter, 's', SpellOp::SwapStartEitherWay, "subc", "susc"},
    SpellEntry{SpellTable::ByFirstLetter, 's', SpellOp::SwapStartEitherWay, "succ", "susc"},
    SpellEntry{SpellTable::ByFirstLetter, 's', SpellOp::SwapStartEitherWay, "subt", "supt"},
    SpellEntry{SpellTable::ByFirstLetter, 's', SpellOp::SwapStartEitherWay, "subt", "sust"},
    SpellEntry{SpellTable::ByFirstLetter, 't', SpellOp::SwapStartEitherWay, "transv", "trav"},
    SpellEntry{SpellTable::ByFirstLetter, 'u', SpellOp::ReplaceStart, "ul", "hul"},
    SpellEntry{SpellTable::ByFirstLetter, 'u', SpellOp::ReplaceStart, "uol", "vul"},
    SpellEntry{SpellTable::ByFirstLetter, 'y', SpellOp::ReplaceStart, "y", "i"},
    SpellEntry{SpellTable::ByFirstLetter, 'z', SpellOp::ReplaceStart, "z", "di"},
    // Any_Tricks.
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "ae", "e"},
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "bul", "bol"},
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "bol", "bul"},
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "cl", "cul"},
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "cu", "quu"},
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "f", "ph"},
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "ph", "f"},
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "h", ""},
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "oe", "e"},
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "vul", "vol"},
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "vol", "vul"},
    SpellEntry{SpellTable::Anywhere, 0, SpellOp::ReplaceAnywhere, "uol", "vul"},
    // Mediaeval_Tricks.
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "col", "caul"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "e", "ae"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "o", "u"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "i", "y"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "ism", "sm"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "isp", "sp"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "ist", "st"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "iz", "z"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "esm", "sm"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "esp", "sp"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "est", "st"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "ez", "z"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "di", "z"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "f", "ph"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "is", "ix"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "b", "p"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "d", "t"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "v", "b"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "v", "f"},
    // NOTE: Twice in WORDS' table; the second never answers, but holds a rule id.
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "v", "f"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "s", "x"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "ci", "ti"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "nt", "nct"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "s", "ns"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "ch", "c"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "c", "ch"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "th", "t"},
    SpellEntry{SpellTable::Medieval, 0, SpellOp::ReplaceAnywhere, "t", "th"},
    // Try_Tricks' own: is -> iis for eo, a terminal iis, a doubled consonant (Double_Consonants, by letter).
    SpellEntry{SpellTable::OwnSteps, 'i', SpellOp::StartIsAsIis, "is", "iis"},
    SpellEntry{SpellTable::OwnSteps, 0, SpellOp::EndIsAsIis, "iis", "is"},
    SpellEntry{SpellTable::OwnSteps, 'b', SpellOp::DoubleConsonant, "b", "bb"},
    SpellEntry{SpellTable::OwnSteps, 'c', SpellOp::DoubleConsonant, "c", "cc"},
    SpellEntry{SpellTable::OwnSteps, 'd', SpellOp::DoubleConsonant, "d", "dd"},
    SpellEntry{SpellTable::OwnSteps, 'f', SpellOp::DoubleConsonant, "f", "ff"},
    SpellEntry{SpellTable::OwnSteps, 'g', SpellOp::DoubleConsonant, "g", "gg"},
    SpellEntry{SpellTable::OwnSteps, 'h', SpellOp::DoubleConsonant, "h", "hh"},
    SpellEntry{SpellTable::OwnSteps, 'j', SpellOp::DoubleConsonant, "j", "jj"},
    SpellEntry{SpellTable::OwnSteps, 'k', SpellOp::DoubleConsonant, "k", "kk"},
    SpellEntry{SpellTable::OwnSteps, 'l', SpellOp::DoubleConsonant, "l", "ll"},
    SpellEntry{SpellTable::OwnSteps, 'm', SpellOp::DoubleConsonant, "m", "mm"},
    SpellEntry{SpellTable::OwnSteps, 'n', SpellOp::DoubleConsonant, "n", "nn"},
    SpellEntry{SpellTable::OwnSteps, 'p', SpellOp::DoubleConsonant, "p", "pp"},
    SpellEntry{SpellTable::OwnSteps, 'q', SpellOp::DoubleConsonant, "q", "qq"},
    SpellEntry{SpellTable::OwnSteps, 'r', SpellOp::DoubleConsonant, "r", "rr"},
    SpellEntry{SpellTable::OwnSteps, 's', SpellOp::DoubleConsonant, "s", "ss"},
    SpellEntry{SpellTable::OwnSteps, 't', SpellOp::DoubleConsonant, "t", "tt"},
    SpellEntry{SpellTable::OwnSteps, 'v', SpellOp::DoubleConsonant, "v", "vv"},
    SpellEntry{SpellTable::OwnSteps, 'w', SpellOp::DoubleConsonant, "w", "ww"},
    SpellEntry{SpellTable::OwnSteps, 'x', SpellOp::DoubleConsonant, "x", "xx"},
    SpellEntry{SpellTable::OwnSteps, 'z', SpellOp::DoubleConsonant, "z", "zz"},
};

// NOTE: UnassimilatePrefix keeps the id of the way WORDS never takes, so rule ids stay as published.
[[nodiscard]] constexpr bool twoWays(SpellOp op) noexcept {
  return op == SpellOp::SwapStartFirstMatch || op == SpellOp::UnassimilatePrefix || op == SpellOp::SwapStartEitherWay;
}
// INFO: A rule is an entry and a way: 1 + the count of ways before it, plus 1 when the word began with the second.
inline constexpr auto kFirstRule = [] {
  std::array<std::uint16_t, kSpellings.size() + 1> first{};
  for (std::size_t i = 0; i < kSpellings.size(); ++i)
    first[i + 1] = static_cast<std::uint16_t>(first[i] + (twoWays(kSpellings[i].op) ? 2 : 1));
  return first;
}();
static_assert(kFirstRule.back() < 256, "a rule id is one byte");
[[nodiscard]] constexpr std::uint8_t ruleOf(std::size_t entry, bool reversed) noexcept {
  return static_cast<std::uint8_t>(1 + kFirstRule[entry] + (reversed ? 1 : 0));
}

} // namespace whitaker::parse
