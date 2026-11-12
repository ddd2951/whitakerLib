#pragma once

#include <type_traits>
#include <utility>

// INFO: WORDS' terms, as ADDONS.LAT and DICTLINE use them:
//  - prefix: added to the start of a stem to change its meaning: ante + cedo -> antecedo.
//  - suffix: added to the end of a stem, before the inflection; it can change the part: vinc- + tor -> victor.
//  - tackon: added to the end of a whole, inflected word; taking it off leaves the same word: -que, -ne, -ve.
//  - packon: a tackon that only joins the qu- pronouns: qui + cumque -> quicumque (DICTLINE's PACK entries).
//  - tickon: added to the front of those same qu- pronouns: ec- + quis -> ecquis.
//  - stem key: which of an entry's four stems an inflection joins; each INFLECTS row names the key it needs.
//  - allowed stem (AllowSet): WORDS' further checks on whether an inflection may join a stem (dic, duc, fac, fer).

// NOTE: Values that are WORDS' own words, its tokens (NOM, PERF) and its rule names (Syncope's IS, SpellingKind's
//  SLURY_SWAP), keep WORDS' spelling: gen matches the tokens against WORDS' text by name, and a reader of WORDS
//  knows them so. The library's own enums (AddonKind here, Status in whitaker.hpp) are camelCase.

// INFO: Read only by codegen; the declarations remain C++23 for library callers.
#ifdef __cpp_impl_reflection
namespace latin::tag {
struct Limit {
  unsigned maximum{};
};
struct Slot {
  unsigned byte{}, shift{};
};
} // namespace latin::tag
#define LATIN_LIMIT(n) [[= ::latin::tag::Limit{n}]]
#define LATIN_SLOT(b, s) [[= ::latin::tag::Slot{b, s}]]
#else
#define LATIN_LIMIT(n)
#define LATIN_SLOT(b, s)
#endif

namespace latin {

// INFO: DICTLINE and ADDONS.LAT number the parts differently. Both are converted to this one ordinal, so the image
//  compares like with like.
enum class Part : unsigned char {
  // INFO: ADDONS.LAT's X: the record accepts any part.
  X = 0,
  N = 1,
  PRON = 2,
  V = 3,
  ADJ = 4,
  ADV = 5,
  PREP = 6,
  NUM = 7,
  CONJ = 8,
  INTERJ = 9,
  PACK = 10,
  SUPINE = 11,
  VPAR = 12,
  NONE = 13,
};

// INFO: ADDONS.LAT's record kinds, in the order the addon table stores them.
enum class AddonKind : unsigned char {
  tickon = 0,
  prefix = 1,
  suffix = 2,
  tackon = 3,
  packon = 4,
};

// INFO: The first way a reading was found, as WORDS reports it; a syncope's rule is in the reading's Derivation.
enum class Route : unsigned char {
  direct = 0,
  syncope = 1,
  adverb = 2,
  slur = 3,
  wordMod = 4,
};

// INFO: WORDS' syncope rules, how a contracted perfect was restored.
enum class Syncope : unsigned char {
  NONE = 0,
  II = 1,
  S = 2,
  R = 3,
  IER = 4,
  IS = 5,
};

// INFO: The WORDS spelling operation behind a respelling rule.
enum class SpellingKind : unsigned char {
  SLURY_SWAP = 1,
  SLUR = 2,
  REPLACED = 3,
  SWAP = 4,
  INTERNAL = 5,
  EO_IIS = 6,
  TERMINAL_IIS = 7,
  DOUBLED = 8,
};

enum class Case : unsigned char {
  X = 0,
  NOM = 1,
  VOC = 2,
  GEN = 3,
  LOC = 4,
  DAT = 5,
  ABL = 6,
  ACC = 7,
};

enum class Number : unsigned char {
  X = 0,
  S = 1,
  P = 2,
};

enum class Gender : unsigned char {
  X = 0,
  M = 1,
  F = 2,
  N = 3,
  C = 4,
};

enum class Tense : unsigned char {
  X = 0,
  PRES = 1,
  IMPF = 2,
  FUT = 3,
  PERF = 4,
  PLUP = 5,
  FUTP = 6,
};

enum class Voice : unsigned char {
  X = 0,
  ACTIVE = 1,
  PASSIVE = 2,
};

enum class Mood : unsigned char {
  X = 0,
  IND = 1,
  SUB = 2,
  IMP = 3,
  INF = 4,
  PPL = 5,
};

enum class Comparison : unsigned char {
  X = 0,
  POS = 1,
  COMP = 2,
  SUPER = 3,
};

// NOTE: WORDS' noun kinds are case-sensitive: P and p are different kinds, so nothing may fold their case.
enum class NounKind : unsigned char {
  X = 0,
  S = 1,
  M = 2,
  A = 3,
  G = 4,
  N = 5,
  P = 6,
  T = 7,
  L = 8,
  W = 9,
  p = 10,
  t = 11,
  w = 12,
  x = 13,
};

enum class PronounKind : unsigned char {
  X = 0,
  PERS = 1,
  REL = 2,
  REFLEX = 3,
  DEMONS = 4,
  INTERR = 5,
  INDEF = 6,
  ADJECT = 7,
};

enum class PackonKind : unsigned char {
  X = 0,
  INTERR = 1,
  REL = 2,
  INDEF = 3,
  ADJECT = 4,
};

// INFO: TO_BE is used only by the synthesized esse entry.
enum class VerbKind : unsigned char {
  X = 0,
  TO_BE = 1,
  TO_BEING = 2,
  GEN = 3,
  DAT = 4,
  ABL = 5,
  TRANS = 6,
  INTRANS = 7,
  IMPERS = 8,
  DEP = 9,
  SEMIDEP = 10,
  PERFDEF = 11,
};

enum class NumeralSort : unsigned char {
  X = 0,
  CARD = 1,
  ORD = 2,
  DIST = 3,
  ADVERB = 4,
};

// INFO: WORDS' one-letter codes, as DICTLINE writes them; WORDS' own documentation says what each letter means.
enum class Age : unsigned char {
  X = 0,
  A = 1,
  B = 2,
  C = 3,
  D = 4,
  E = 5,
  F = 6,
  G = 7,
  H = 8,
};

enum class Frequency : unsigned char {
  X = 0,
  A = 1,
  B = 2,
  C = 3,
  D = 4,
  E = 5,
  F = 6,
  I = 7,
  M = 8,
  N = 9,
};

enum class Area : unsigned char {
  X = 0,
  A = 1,
  B = 2,
  D = 3,
  E = 4,
  G = 5,
  L = 6,
  P = 7,
  S = 8,
  T = 9,
  W = 10,
  Y = 11,
};

enum class Geography : unsigned char {
  X = 0,
  A = 1,
  B = 2,
  C = 3,
  D = 4,
  E = 5,
  F = 6,
  G = 7,
  H = 8,
  I = 9,
  J = 10,
  K = 11,
  N = 12,
  P = 13,
  Q = 14,
  R = 15,
  S = 16,
  U = 17,
};

enum class Source : unsigned char {
  X = 0,
  A = 1,
  B = 2,
  C = 3,
  D = 4,
  E = 5,
  F = 6,
  G = 7,
  H = 8,
  I = 9,
  J = 10,
  K = 11,
  L = 12,
  M = 13,
  N = 14,
  O = 15,
  P = 16,
  Q = 17,
  R = 18,
  S = 19,
  T = 20,
  U = 21,
  V = 22,
  W = 23,
  Y = 24,
  Z = 25,
};

inline constexpr unsigned kDeclensionMaximum = 9;
struct LATIN_LIMIT(kDeclensionMaximum) Declension {
  unsigned char value{};
  friend constexpr bool operator==(Declension, Declension) = default;
};
static_assert(std::is_trivially_copyable_v<Declension>);

inline constexpr unsigned kVariantMaximum = 9;
struct LATIN_LIMIT(kVariantMaximum) Variant {
  unsigned char value{};
  friend constexpr bool operator==(Variant, Variant) = default;
};
static_assert(std::is_trivially_copyable_v<Variant>);

inline constexpr unsigned kConjugationMaximum = 9;
struct LATIN_LIMIT(kConjugationMaximum) Conjugation {
  unsigned char value{};
  friend constexpr bool operator==(Conjugation, Conjugation) = default;
};
static_assert(std::is_trivially_copyable_v<Conjugation>);

inline constexpr unsigned kStemKeyMaximum = 9;
struct LATIN_LIMIT(kStemKeyMaximum) StemKey {
  unsigned char value{};
  friend constexpr bool operator==(StemKey, StemKey) = default;
};
static_assert(std::is_trivially_copyable_v<StemKey>);

inline constexpr unsigned kCharacterCountMaximum = 7;
struct LATIN_LIMIT(kCharacterCountMaximum) CharacterCount {
  unsigned char value{};
  friend constexpr bool operator==(CharacterCount, CharacterCount) = default;
};
static_assert(std::is_trivially_copyable_v<CharacterCount>);

inline constexpr unsigned kPersonMaximum = 3;
struct LATIN_LIMIT(kPersonMaximum) Person {
  unsigned char value{};
  friend constexpr bool operator==(Person, Person) = default;
};
static_assert(std::is_trivially_copyable_v<Person>);

// INFO: EntryNumeralValue is a DICTLINE numeral's own value, up to 1000; NumeralValue is a Roman numeral read from
//  the text, up to 4000.
inline constexpr unsigned kEntryNumeralValueMaximum = 1000;
struct LATIN_LIMIT(kEntryNumeralValueMaximum) EntryNumeralValue {
  unsigned short value{};
  friend constexpr bool operator==(EntryNumeralValue, EntryNumeralValue) = default;
};
static_assert(std::is_trivially_copyable_v<EntryNumeralValue>);

inline constexpr unsigned kNumeralValueMaximum = 4000;
struct LATIN_LIMIT(kNumeralValueMaximum) NumeralValue {
  unsigned short value{};
  friend constexpr bool operator==(NumeralValue, NumeralValue) = default;
};
static_assert(std::is_trivially_copyable_v<NumeralValue>);

// INFO: A declension or a conjugation, which by the part of speech.
inline constexpr unsigned kWhichMaximum =
    kDeclensionMaximum > kConjugationMaximum ? kDeclensionMaximum : kConjugationMaximum;
struct LATIN_LIMIT(kWhichMaximum) Which {
  unsigned char value{};
  friend constexpr bool operator==(Which, Which) = default;
};
static_assert(std::is_trivially_copyable_v<Which>);

// INFO: A NounKind, PronounKind, PackonKind or VerbKind, which by the part of speech.
inline constexpr unsigned kKindMaximum = 13;
struct LATIN_LIMIT(kKindMaximum) Kind {
  unsigned char value{};
  friend constexpr bool operator==(Kind, Kind) = default;
};
static_assert(std::is_trivially_copyable_v<Kind>);
static_assert(std::to_underlying(NounKind::x) <= kKindMaximum && std::to_underlying(VerbKind::PERFDEF) <= kKindMaximum);

struct Analysis {
  Part part{};
  LATIN_SLOT(0, 4) Which which {};
  LATIN_SLOT(0, 0) Variant variant {};
  LATIN_SLOT(1, 5) Case caseOf {};
  LATIN_SLOT(1, 3) Number number {};
  LATIN_SLOT(1, 0) Gender gender {};
  LATIN_SLOT(2, 0) Comparison comparison {};
  LATIN_SLOT(2, 0) NumeralSort numeralSort {};
  LATIN_SLOT(2, 5) Tense tense {};
  LATIN_SLOT(2, 3) Voice voice {};
  LATIN_SLOT(2, 0) Mood mood {};
  LATIN_SLOT(1, 0) Person person {};
  friend constexpr bool operator==(const Analysis&, const Analysis&) = default;
};
static_assert(std::is_trivially_copyable_v<Analysis>);

// INFO: What the dictionary says of a word, which no ending changes: WORDS' Part_Entry and Translation_Record. Only
//  the fields its part has are set, the rest 0.
struct DictionaryWord {
  Part part{};
  Which which{};
  Variant variant{};
  Gender gender{};
  Kind kind{};
  Comparison comparison{};
  NumeralSort numeralSort{};
  NumeralValue numeralValue{};
  Age age{};
  Area area{};
  Geography geography{};
  Frequency frequency{};
  Source source{};
  friend constexpr bool operator==(const DictionaryWord&, const DictionaryWord&) = default;
};
static_assert(std::is_trivially_copyable_v<DictionaryWord>);

} // namespace latin

#undef LATIN_LIMIT
#undef LATIN_SLOT
