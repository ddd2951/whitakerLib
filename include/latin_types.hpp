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

// NOTE: Values a reader sees are named in the reader's words and tagged with WORDS' token (nominative is NOM):
//  gen matches the token against WORDS' text, and latin::name gives it back. Values that are only WORDS' own, its
//  rule names (Syncope's IS, SpellingKind's SLURY_SWAP) and one-letter codes (Age, NounKind), keep WORDS' spelling
//  and need no tag. The library's own enums (AddonKind here, Status in whitaker.hpp) are camelCase.

// INFO: Read only by codegen; the declarations remain C++23 for library callers.
#ifdef __cpp_impl_reflection
namespace latin::tag {
struct Limit {
  unsigned maximum{};
};
struct Slot {
  unsigned byte{}, shift{};
};
// NOTE: An array, not a pointer: a pointer to a literal cannot be an annotation's value.
struct Words {
  char token[9]{};
};
} // namespace latin::tag
#define LATIN_LIMIT(n) [[= ::latin::tag::Limit{n}]]
#define LATIN_SLOT(b, s) [[= ::latin::tag::Slot{b, s}]]
#define LATIN_WORDS(t) [[= ::latin::tag::Words{t}]]
#else
#define LATIN_LIMIT(n)
#define LATIN_SLOT(b, s)
#define LATIN_WORDS(t)
#endif

namespace latin {

// INFO: DICTLINE and ADDONS.LAT number the parts differently. Both are converted to this one ordinal, so the image
//  compares like with like.
enum class Part : unsigned char {
  // INFO: ADDONS.LAT's X: the record accepts any part.
  any LATIN_WORDS("X") = 0,
  noun LATIN_WORDS("N") = 1,
  pronoun LATIN_WORDS("PRON") = 2,
  verb LATIN_WORDS("V") = 3,
  adjective LATIN_WORDS("ADJ") = 4,
  adverb LATIN_WORDS("ADV") = 5,
  preposition LATIN_WORDS("PREP") = 6,
  numeral LATIN_WORDS("NUM") = 7,
  conjunction LATIN_WORDS("CONJ") = 8,
  interjection LATIN_WORDS("INTERJ") = 9,
  packon LATIN_WORDS("PACK") = 10,
  supine LATIN_WORDS("SUPINE") = 11,
  participle LATIN_WORDS("VPAR") = 12,
  none LATIN_WORDS("NONE") = 13,
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
  any LATIN_WORDS("X") = 0,
  nominative LATIN_WORDS("NOM") = 1,
  vocative LATIN_WORDS("VOC") = 2,
  genitive LATIN_WORDS("GEN") = 3,
  locative LATIN_WORDS("LOC") = 4,
  dative LATIN_WORDS("DAT") = 5,
  ablative LATIN_WORDS("ABL") = 6,
  accusative LATIN_WORDS("ACC") = 7,
};

enum class Number : unsigned char {
  any LATIN_WORDS("X") = 0,
  singular LATIN_WORDS("S") = 1,
  plural LATIN_WORDS("P") = 2,
};

enum class Gender : unsigned char {
  any LATIN_WORDS("X") = 0,
  masculine LATIN_WORDS("M") = 1,
  feminine LATIN_WORDS("F") = 2,
  neuter LATIN_WORDS("N") = 3,
  common LATIN_WORDS("C") = 4,
};

enum class Tense : unsigned char {
  any LATIN_WORDS("X") = 0,
  present LATIN_WORDS("PRES") = 1,
  imperfect LATIN_WORDS("IMPF") = 2,
  future LATIN_WORDS("FUT") = 3,
  perfect LATIN_WORDS("PERF") = 4,
  pluperfect LATIN_WORDS("PLUP") = 5,
  futurePerfect LATIN_WORDS("FUTP") = 6,
};

enum class Voice : unsigned char {
  any LATIN_WORDS("X") = 0,
  active LATIN_WORDS("ACTIVE") = 1,
  passive LATIN_WORDS("PASSIVE") = 2,
};

enum class Mood : unsigned char {
  any LATIN_WORDS("X") = 0,
  indicative LATIN_WORDS("IND") = 1,
  subjunctive LATIN_WORDS("SUB") = 2,
  imperative LATIN_WORDS("IMP") = 3,
  infinitive LATIN_WORDS("INF") = 4,
  participle LATIN_WORDS("PPL") = 5,
};

enum class Comparison : unsigned char {
  any LATIN_WORDS("X") = 0,
  positive LATIN_WORDS("POS") = 1,
  comparative LATIN_WORDS("COMP") = 2,
  superlative LATIN_WORDS("SUPER") = 3,
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
  any LATIN_WORDS("X") = 0,
  personal LATIN_WORDS("PERS") = 1,
  relative LATIN_WORDS("REL") = 2,
  reflexive LATIN_WORDS("REFLEX") = 3,
  demonstrative LATIN_WORDS("DEMONS") = 4,
  interrogative LATIN_WORDS("INTERR") = 5,
  indefinite LATIN_WORDS("INDEF") = 6,
  adjectival LATIN_WORDS("ADJECT") = 7,
};

enum class PackonKind : unsigned char {
  any LATIN_WORDS("X") = 0,
  interrogative LATIN_WORDS("INTERR") = 1,
  relative LATIN_WORDS("REL") = 2,
  indefinite LATIN_WORDS("INDEF") = 3,
  adjectival LATIN_WORDS("ADJECT") = 4,
};

// INFO: TO_BE is used only by the synthesized esse entry.
enum class VerbKind : unsigned char {
  any LATIN_WORDS("X") = 0,
  toBe LATIN_WORDS("TO_BE") = 1,
  toBeing LATIN_WORDS("TO_BEING") = 2,
  genitive LATIN_WORDS("GEN") = 3,
  dative LATIN_WORDS("DAT") = 4,
  ablative LATIN_WORDS("ABL") = 5,
  transitive LATIN_WORDS("TRANS") = 6,
  intransitive LATIN_WORDS("INTRANS") = 7,
  impersonal LATIN_WORDS("IMPERS") = 8,
  deponent LATIN_WORDS("DEP") = 9,
  semideponent LATIN_WORDS("SEMIDEP") = 10,
  perfectDefinite LATIN_WORDS("PERFDEF") = 11,
};

enum class NumeralSort : unsigned char {
  any LATIN_WORDS("X") = 0,
  cardinal LATIN_WORDS("CARD") = 1,
  ordinal LATIN_WORDS("ORD") = 2,
  distributive LATIN_WORDS("DIST") = 3,
  adverbial LATIN_WORDS("ADVERB") = 4,
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
static_assert(std::to_underlying(NounKind::x) <= kKindMaximum &&
              std::to_underlying(VerbKind::perfectDefinite) <= kKindMaximum);

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
