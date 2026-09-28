#pragma once

#include <cstddef>

namespace latin {

// INFO: DICTLINE and ADDONS.LAT number the parts differently. Both are
//  converted to this one ordinal, so the image compares like with like.
enum class Part : unsigned char {
  // ADDONS.LAT's X: the record accepts any part.
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

// ADDONS.LAT's record kinds, in the order the addon table stores them.
enum class AddonKind : unsigned char {
  Tickon = 0,
  Prefix = 1,
  Suffix = 2,
  Tackon = 3,
  Packon = 4,
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

// NOTE: Keep upper and lower case distinct until emission.
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

struct Declension {
  unsigned char value{};
  friend constexpr bool operator==(Declension, Declension) = default;
};

struct Variant {
  unsigned char value{};
  friend constexpr bool operator==(Variant, Variant) = default;
};

struct Conjugation {
  unsigned char value{};
  friend constexpr bool operator==(Conjugation, Conjugation) = default;
};

struct StemKey {
  unsigned char value{};
  friend constexpr bool operator==(StemKey, StemKey) = default;
};

struct CharacterCount {
  unsigned char value{};
  friend constexpr bool operator==(CharacterCount, CharacterCount) = default;
};

struct Person {
  unsigned char value{};
  friend constexpr bool operator==(Person, Person) = default;
};

struct NumeralValue {
  unsigned short value{};
  friend constexpr bool operator==(NumeralValue, NumeralValue) = default;
};

struct Analysis {
  Part part{};
  unsigned char which{};
  Variant variant{};
  Case caseOf{};
  Number number{};
  Gender gender{};
  Comparison comparison{};
  NumeralSort numeralSort{};
  Tense tense{};
  Voice voice{};
  Mood mood{};
  Person person{};
  friend constexpr bool operator==(const Analysis&, const Analysis&) = default;
};

// INFO: Declensions 7 and 8 are not part of the source domain.
[[nodiscard]] constexpr bool isValid(Declension d) noexcept {
  return d.value <= 6 || d.value == 9;
}
[[nodiscard]] constexpr bool isValid(Variant v) noexcept {
  return v.value <= 9;
}
[[nodiscard]] constexpr bool isValid(Conjugation c) noexcept {
  return c.value <= 9;
}
[[nodiscard]] constexpr bool isValid(StemKey key) noexcept {
  return key.value <= 9;
}
[[nodiscard]] constexpr bool isValid(CharacterCount count) noexcept {
  return count.value <= 7;
}
[[nodiscard]] constexpr bool isValid(Person person) noexcept {
  return person.value <= 3;
}
// INFO: Whitaker's NUMERAL_VALUE_TYPE is range 0..1000.
[[nodiscard]] constexpr bool isValid(NumeralValue value) noexcept {
  return value.value <= 1000;
}

[[nodiscard]] constexpr bool isPart(unsigned char value) noexcept {
  switch (Part{value}) {
  case Part::X:
  case Part::N:
  case Part::PRON:
  case Part::V:
  case Part::ADJ:
  case Part::ADV:
  case Part::PREP:
  case Part::NUM:
  case Part::CONJ:
  case Part::INTERJ:
  case Part::PACK:
  case Part::SUPINE:
  case Part::VPAR:
  case Part::NONE:
    return true;
  }
  return false;
}

[[nodiscard]] constexpr bool isAddonKind(unsigned char value) noexcept {
  switch (AddonKind{value}) {
  case AddonKind::Tickon:
  case AddonKind::Prefix:
  case AddonKind::Suffix:
  case AddonKind::Tackon:
  case AddonKind::Packon:
    return true;
  }
  return false;
}

// INFO: word_package.adb:854-858; a prefix's root of X accepts any part.
[[nodiscard]] constexpr bool rootAdmits(Part root, Part part) noexcept {
  return root == Part::X || root == part;
}

// INFO: word_package.adb:756-765 and 819, Reduce_Stem_List's "<=" on parts.
[[nodiscard]] constexpr bool suffixRootAdmits(Part root, Part part) noexcept {
  return rootAdmits(root, part) || (part == Part::PACK && root == Part::PRON);
}

// INFO: word_package.adb:780-787, 820-823 and 884-887; the asymmetry is
//  Whitaker's.
[[nodiscard]] constexpr bool
stemKeyAdmits(unsigned char dictKey, unsigned char wanted, Part part) noexcept {
  if (dictKey == wanted || wanted == 0)
    return true;
  return dictKey == 0 && wanted >= 1 && wanted <= 2 &&
         (part == Part::N || part == Part::ADJ || part == Part::V);
}

// INFO: Whitaker's matching predicates, dictionary on the left, inflection on
//  the right.
// INFO: inflections_package.adb:266-277, "<=" on Decn_Record, which holds a
//  declension or a conjugation.
[[nodiscard]] constexpr bool
numberMatches(unsigned char left, unsigned char leftVariant,
              unsigned char right, unsigned char rightVariant) noexcept {
  if (right == left && rightVariant == leftVariant)
    return true;
  if (right == 0 && rightVariant == 0 && left != 9)
    return true;
  return right == left && rightVariant == 0;
}

[[nodiscard]] constexpr bool declensionMatches(Declension left,
                                               Variant leftVariant,
                                               Declension right,
                                               Variant rightVariant) noexcept {
  return numberMatches(left.value, leftVariant.value, right.value,
                       rightVariant.value);
}

[[nodiscard]] constexpr bool conjugationMatches(Conjugation left,
                                                Variant leftVariant,
                                                Conjugation right,
                                                Variant rightVariant) noexcept {
  return numberMatches(left.value, leftVariant.value, right.value,
                       rightVariant.value);
}

// INFO: word_package.adb:768-778, Reduce_Stem_List's own "<=" on genders.
[[nodiscard]] constexpr bool genderMatches(Gender left, Gender right) noexcept {
  return right == left || right == Gender::X ||
         (right == Gender::C && left != Gender::N);
}

// INFO: word_package.adb:944-946.
[[nodiscard]] constexpr bool comparisonMatches(Comparison left,
                                               Comparison right) noexcept {
  return right == left || right == Comparison::X || left == Comparison::X;
}

// INFO: addons_package.adb:122-125; a TACKON whose entry is a PACK of
//  declension 1 or 2 and whose gloss starts "PACKON w/" is a PACKON.
template <typename Gloss>
[[nodiscard]] constexpr bool isPackon(Declension declension,
                                      const Gloss& gloss) noexcept {
  return (declension.value == 1 || declension.value == 2) &&
         gloss.starts_with("PACKON w/");
}

// INFO: list_sweep.adb:362-378 and 401-415, List_Sweep's rarity pass.
[[nodiscard]] constexpr bool archaic(Age age) noexcept { return age == Age::A; }

[[nodiscard]] constexpr bool medieval(Age age) noexcept {
  switch (age) {
  case Age::F:
  case Age::G:
  case Age::H:
    return true;
  case Age::X:
  case Age::A:
  case Age::B:
  case Age::C:
  case Age::D:
  case Age::E:
    return false;
  }
  return false;
}

[[nodiscard]] constexpr bool uncommonInflection(Frequency frequency) noexcept {
  switch (frequency) {
  case Frequency::X:
  case Frequency::A:
  case Frequency::B:
    return false;
  case Frequency::C:
  case Frequency::D:
  case Frequency::E:
  case Frequency::F:
  case Frequency::I:
  case Frequency::M:
  case Frequency::N:
    return true;
  }
  return false;
}

[[nodiscard]] constexpr bool uncommonEntry(Frequency frequency) noexcept {
  return uncommonInflection(frequency) && frequency != Frequency::C;
}

// INFO: Allowed_Stem's mood and tense ranges (list_sweep.adb:29-189).
[[nodiscard]] constexpr bool moodIndToInf(Mood mood) noexcept {
  return mood == Mood::IND || mood == Mood::SUB || mood == Mood::IMP ||
         mood == Mood::INF;
}

[[nodiscard]] constexpr bool moodIndToImp(Mood mood) noexcept {
  return mood == Mood::IND || mood == Mood::SUB || mood == Mood::IMP;
}

[[nodiscard]] constexpr bool tensePresToFut(Tense tense) noexcept {
  return tense == Tense::PRES || tense == Tense::IMPF || tense == Tense::FUT;
}

[[nodiscard]] constexpr bool tensePerfToFutp(Tense tense) noexcept {
  return tense == Tense::PERF || tense == Tense::PLUP || tense == Tense::FUTP;
}

// An inflection row's share of Allowed_Stem; the entry's share is its verb
// kind and the stem the reading prints.
struct AllowSet {
  enum Bit : unsigned char {
    IsVerb = 1 << 0,
    ShortImp = 1 << 1,
    ImpNoPerson = 1 << 2,
    NotThird = 1 << 3,
    DepKeep = 1 << 4,
    DepDrop = 1 << 5,
    SemidepDrop = 1 << 6,
  };
  unsigned char bits{};
  [[nodiscard]] constexpr bool has(Bit bit) const noexcept {
    return (bits & bit) != 0;
  }
  constexpr void add(Bit bit) noexcept { bits |= bit; }
};

[[nodiscard]] constexpr AllowSet allowOf(Tense tense, Voice voice, Mood mood,
                                         Person person, Number number,
                                         bool noEnding) noexcept {
  AllowSet allow{};
  allow.add(AllowSet::IsVerb);
  if (tense == Tense::PRES && voice == Voice::ACTIVE && mood == Mood::IMP &&
      person.value == 2 && number == Number::S && noEnding)
    allow.add(AllowSet::ShortImp);
  if (mood == Mood::IMP &&
      !((tense == Tense::PRES && person.value == 2) ||
        (tense == Tense::FUT && (person.value == 2 || person.value == 3))))
    allow.add(AllowSet::ImpNoPerson);
  if (person.value != 3)
    allow.add(AllowSet::NotThird);
  if (voice == Voice::ACTIVE && mood == Mood::INF && tense == Tense::FUT)
    allow.add(AllowSet::DepKeep);
  else if (voice == Voice::ACTIVE && moodIndToInf(mood))
    allow.add(AllowSet::DepDrop);
  if (moodIndToImp(mood) &&
      ((voice == Voice::PASSIVE && tensePresToFut(tense)) ||
       (voice == Voice::ACTIVE && tensePerfToFutp(tense))))
    allow.add(AllowSet::SemidepDrop);
  return allow;
}

// INFO: dic/duc/fac/fer shortened imperative (G&L 130.5).
template <typename Stem>
[[nodiscard]] constexpr bool allowedStem(AllowSet allow, VerbKind kind,
                                         bool conjugationThreeOne,
                                         const Stem& stem) noexcept {
  if (!allow.has(AllowSet::IsVerb))
    return true;
  bool allowed = true;
  if (allow.has(AllowSet::ShortImp) && conjugationThreeOne)
    allowed = stem.ends_with("dic") || stem.ends_with("duc") ||
              stem.ends_with("fac") || stem.ends_with("fer");
  if (allow.has(AllowSet::ImpNoPerson))
    allowed = false;
  if (kind == VerbKind::IMPERS && allow.has(AllowSet::NotThird))
    allowed = false;
  // INFO: Whitaker's order: the future active infinitive restores a reading.
  if (kind == VerbKind::DEP) {
    if (allow.has(AllowSet::DepKeep))
      allowed = true;
    else if (allow.has(AllowSet::DepDrop))
      allowed = false;
  }
  // INFO: Semi-deponents are deponent only in the perfect system.
  if (kind == VerbKind::SEMIDEP && allow.has(AllowSet::SemidepDrop))
    allowed = false;
  return allowed;
}

[[nodiscard]] constexpr const char* name(Part value) noexcept {
  switch (value) {
  case Part::X:
    return "X";
  case Part::N:
    return "N";
  case Part::PRON:
    return "PRON";
  case Part::V:
    return "V";
  case Part::ADJ:
    return "ADJ";
  case Part::ADV:
    return "ADV";
  case Part::PREP:
    return "PREP";
  case Part::NUM:
    return "NUM";
  case Part::CONJ:
    return "CONJ";
  case Part::INTERJ:
    return "INTERJ";
  case Part::PACK:
    return "PACK";
  case Part::SUPINE:
    return "SUPINE";
  case Part::VPAR:
    return "VPAR";
  case Part::NONE:
    return "NONE";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Case value) noexcept {
  switch (value) {
  case Case::X:
    return "X";
  case Case::NOM:
    return "NOM";
  case Case::VOC:
    return "VOC";
  case Case::GEN:
    return "GEN";
  case Case::LOC:
    return "LOC";
  case Case::DAT:
    return "DAT";
  case Case::ABL:
    return "ABL";
  case Case::ACC:
    return "ACC";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Number value) noexcept {
  switch (value) {
  case Number::X:
    return "X";
  case Number::S:
    return "S";
  case Number::P:
    return "P";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Gender value) noexcept {
  switch (value) {
  case Gender::X:
    return "X";
  case Gender::M:
    return "M";
  case Gender::F:
    return "F";
  case Gender::N:
    return "N";
  case Gender::C:
    return "C";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Comparison value) noexcept {
  switch (value) {
  case Comparison::X:
    return "X";
  case Comparison::POS:
    return "POS";
  case Comparison::COMP:
    return "COMP";
  case Comparison::SUPER:
    return "SUPER";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(NumeralSort value) noexcept {
  switch (value) {
  case NumeralSort::X:
    return "X";
  case NumeralSort::CARD:
    return "CARD";
  case NumeralSort::ORD:
    return "ORD";
  case NumeralSort::DIST:
    return "DIST";
  case NumeralSort::ADVERB:
    return "ADVERB";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Tense value) noexcept {
  switch (value) {
  case Tense::X:
    return "X";
  case Tense::PRES:
    return "PRES";
  case Tense::IMPF:
    return "IMPF";
  case Tense::FUT:
    return "FUT";
  case Tense::PERF:
    return "PERF";
  case Tense::PLUP:
    return "PLUP";
  case Tense::FUTP:
    return "FUTP";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Voice value) noexcept {
  switch (value) {
  case Voice::X:
    return "X";
  case Voice::ACTIVE:
    return "ACTIVE";
  case Voice::PASSIVE:
    return "PASSIVE";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Mood value) noexcept {
  switch (value) {
  case Mood::X:
    return "X";
  case Mood::IND:
    return "IND";
  case Mood::SUB:
    return "SUB";
  case Mood::IMP:
    return "IMP";
  case Mood::INF:
    return "INF";
  case Mood::PPL:
    return "PPL";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(NounKind value) noexcept {
  switch (value) {
  case NounKind::X:
    return "X";
  case NounKind::S:
    return "S";
  case NounKind::M:
    return "M";
  case NounKind::A:
    return "A";
  case NounKind::G:
    return "G";
  case NounKind::N:
    return "N";
  case NounKind::P:
    return "P";
  case NounKind::T:
    return "T";
  case NounKind::L:
    return "L";
  case NounKind::W:
    return "W";
  case NounKind::p:
    return "p";
  case NounKind::t:
    return "t";
  case NounKind::w:
    return "w";
  case NounKind::x:
    return "x";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(PronounKind value) noexcept {
  switch (value) {
  case PronounKind::X:
    return "X";
  case PronounKind::PERS:
    return "PERS";
  case PronounKind::REL:
    return "REL";
  case PronounKind::REFLEX:
    return "REFLEX";
  case PronounKind::DEMONS:
    return "DEMONS";
  case PronounKind::INTERR:
    return "INTERR";
  case PronounKind::INDEF:
    return "INDEF";
  case PronounKind::ADJECT:
    return "ADJECT";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(PackonKind value) noexcept {
  switch (value) {
  case PackonKind::X:
    return "X";
  case PackonKind::INTERR:
    return "INTERR";
  case PackonKind::REL:
    return "REL";
  case PackonKind::INDEF:
    return "INDEF";
  case PackonKind::ADJECT:
    return "ADJECT";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(VerbKind value) noexcept {
  switch (value) {
  case VerbKind::X:
    return "X";
  case VerbKind::TO_BE:
    return "TO_BE";
  case VerbKind::TO_BEING:
    return "TO_BEING";
  case VerbKind::GEN:
    return "GEN";
  case VerbKind::DAT:
    return "DAT";
  case VerbKind::ABL:
    return "ABL";
  case VerbKind::TRANS:
    return "TRANS";
  case VerbKind::INTRANS:
    return "INTRANS";
  case VerbKind::IMPERS:
    return "IMPERS";
  case VerbKind::DEP:
    return "DEP";
  case VerbKind::SEMIDEP:
    return "SEMIDEP";
  case VerbKind::PERFDEF:
    return "PERFDEF";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Age value) noexcept {
  switch (value) {
  case Age::X:
    return "X";
  case Age::A:
    return "A";
  case Age::B:
    return "B";
  case Age::C:
    return "C";
  case Age::D:
    return "D";
  case Age::E:
    return "E";
  case Age::F:
    return "F";
  case Age::G:
    return "G";
  case Age::H:
    return "H";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Frequency value) noexcept {
  switch (value) {
  case Frequency::X:
    return "X";
  case Frequency::A:
    return "A";
  case Frequency::B:
    return "B";
  case Frequency::C:
    return "C";
  case Frequency::D:
    return "D";
  case Frequency::E:
    return "E";
  case Frequency::F:
    return "F";
  case Frequency::I:
    return "I";
  case Frequency::M:
    return "M";
  case Frequency::N:
    return "N";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Area value) noexcept {
  switch (value) {
  case Area::X:
    return "X";
  case Area::A:
    return "A";
  case Area::B:
    return "B";
  case Area::D:
    return "D";
  case Area::E:
    return "E";
  case Area::G:
    return "G";
  case Area::L:
    return "L";
  case Area::P:
    return "P";
  case Area::S:
    return "S";
  case Area::T:
    return "T";
  case Area::W:
    return "W";
  case Area::Y:
    return "Y";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Geography value) noexcept {
  switch (value) {
  case Geography::X:
    return "X";
  case Geography::A:
    return "A";
  case Geography::B:
    return "B";
  case Geography::C:
    return "C";
  case Geography::D:
    return "D";
  case Geography::E:
    return "E";
  case Geography::F:
    return "F";
  case Geography::G:
    return "G";
  case Geography::H:
    return "H";
  case Geography::I:
    return "I";
  case Geography::J:
    return "J";
  case Geography::K:
    return "K";
  case Geography::N:
    return "N";
  case Geography::P:
    return "P";
  case Geography::Q:
    return "Q";
  case Geography::R:
    return "R";
  case Geography::S:
    return "S";
  case Geography::U:
    return "U";
  }
  return nullptr;
}

[[nodiscard]] constexpr const char* name(Source value) noexcept {
  switch (value) {
  case Source::X:
    return "X";
  case Source::A:
    return "A";
  case Source::B:
    return "B";
  case Source::C:
    return "C";
  case Source::D:
    return "D";
  case Source::E:
    return "E";
  case Source::F:
    return "F";
  case Source::G:
    return "G";
  case Source::H:
    return "H";
  case Source::I:
    return "I";
  case Source::J:
    return "J";
  case Source::K:
    return "K";
  case Source::L:
    return "L";
  case Source::M:
    return "M";
  case Source::N:
    return "N";
  case Source::O:
    return "O";
  case Source::P:
    return "P";
  case Source::Q:
    return "Q";
  case Source::R:
    return "R";
  case Source::S:
    return "S";
  case Source::T:
    return "T";
  case Source::U:
    return "U";
  case Source::V:
    return "V";
  case Source::W:
    return "W";
  case Source::Y:
    return "Y";
  case Source::Z:
    return "Z";
  }
  return nullptr;
}

namespace detail {

struct Line {
  char* out;
  std::size_t size;
  std::size_t used{};

  constexpr void put(char c) noexcept {
    if (used + 1 < size)
      out[used] = c;
    ++used;
  }
  constexpr void put(const char* text) noexcept {
    for (text = text != nullptr ? text : "?"; *text != '\0'; ++text)
      put(*text);
  }
  constexpr void digit(unsigned char value) noexcept {
    put(value <= 9 ? "0123456789"[value] : '?');
  }
  constexpr void field(const char* text) noexcept {
    put(' ');
    put(text);
  }
  constexpr void number(char prefix, unsigned char value) noexcept {
    put(' ');
    put(prefix);
    digit(value);
  }
};

} // namespace detail

// INFO: The line WORDS prints for an inflection. Like snprintf: writes at
//  most `size` bytes, the terminator included, and returns the length of
//  the whole line.
constexpr std::size_t describe(const Analysis& analysis, char* out,
                               std::size_t size) noexcept {
  detail::Line line{out, size};
  line.put(name(analysis.part));
  switch (analysis.part) {
  case Part::N:
  case Part::PRON:
  case Part::ADJ:
  case Part::NUM:
    line.number('D', analysis.which);
    line.number('V', analysis.variant.value);
    break;
  case Part::V:
  case Part::VPAR:
  case Part::SUPINE:
    line.number('C', analysis.which);
    line.number('V', analysis.variant.value);
    break;
  default:
    break;
  }

  switch (analysis.part) {
  case Part::N:
  case Part::PRON:
  case Part::SUPINE:
    line.field(name(analysis.caseOf));
    line.field(name(analysis.number));
    line.field(name(analysis.gender));
    break;
  case Part::ADJ:
    line.field(name(analysis.caseOf));
    line.field(name(analysis.number));
    line.field(name(analysis.gender));
    line.field(name(analysis.comparison));
    break;
  case Part::NUM:
    line.field(name(analysis.caseOf));
    line.field(name(analysis.number));
    line.field(name(analysis.gender));
    line.field(name(analysis.numeralSort));
    break;
  case Part::ADV:
    line.field(name(analysis.comparison));
    break;
  case Part::V:
    line.field(name(analysis.tense));
    line.field(name(analysis.voice));
    line.field(name(analysis.mood));
    if (analysis.person.value != 0) {
      line.put(' ');
      line.digit(analysis.person.value);
    }
    line.field(name(analysis.number));
    break;
  case Part::VPAR:
    line.field(name(analysis.caseOf));
    line.field(name(analysis.number));
    line.field(name(analysis.gender));
    line.field(name(analysis.tense));
    line.field(name(analysis.voice));
    line.field(name(analysis.mood));
    break;
  case Part::PREP:
    line.field(name(analysis.caseOf));
    break;
  default:
    break;
  }
  if (size != 0)
    out[line.used < size ? line.used : size - 1] = '\0';
  return line.used;
}

} // namespace latin
