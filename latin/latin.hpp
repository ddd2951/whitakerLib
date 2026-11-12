#pragma once

#include <latin_generated.hpp>

#include <tuple>
#include <type_traits>

namespace latin {

// INFO: Declensions 7 and 8 are not part of the source domain.
[[nodiscard]] constexpr bool isValid(Declension d) noexcept { return d.value <= 6 || d.value == 9; }
[[nodiscard]] constexpr bool isValid(Variant v) noexcept { return v.value <= kVariantMaximum; }
[[nodiscard]] constexpr bool isValid(Conjugation c) noexcept { return c.value <= kConjugationMaximum; }
[[nodiscard]] constexpr bool isValid(StemKey key) noexcept { return key.value <= kStemKeyMaximum; }
[[nodiscard]] constexpr bool isValid(CharacterCount count) noexcept { return count.value <= kCharacterCountMaximum; }
[[nodiscard]] constexpr bool isValid(Person person) noexcept { return person.value <= kPersonMaximum; }
[[nodiscard]] constexpr bool isValid(EntryNumeralValue value) noexcept {
  return value.value <= kEntryNumeralValueMaximum;
}
// INFO: Roman readings have no image entry.
[[nodiscard]] constexpr bool isValid(NumeralValue value) noexcept { return value.value <= kNumeralValueMaximum; }

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
    case AddonKind::tickon:
    case AddonKind::prefix:
    case AddonKind::suffix:
    case AddonKind::tackon:
    case AddonKind::packon:
      return true;
  }
  return false;
}

// INFO: Reduce_Stem_List: a prefix's root of X accepts any part.
[[nodiscard]] constexpr bool rootAdmits(Part root, Part part) noexcept { return root == Part::X || root == part; }

// INFO: Reduce_Stem_List's "<=" on parts.
[[nodiscard]] constexpr bool suffixRootAdmits(Part root, Part part) noexcept {
  return rootAdmits(root, part) || (part == Part::PACK && root == Part::PRON);
}

// INFO: Part_Of_Speech_Type's order.
[[nodiscard]] constexpr unsigned pofsPos(Part part) noexcept {
  switch (part) {
    case Part::X:
      return 0;
    case Part::N:
      return 1;
    case Part::PRON:
      return 2;
    case Part::PACK:
      return 3;
    case Part::ADJ:
      return 4;
    case Part::NUM:
      return 5;
    case Part::ADV:
      return 6;
    case Part::V:
      return 7;
    case Part::VPAR:
      return 8;
    case Part::SUPINE:
      return 9;
    case Part::PREP:
      return 10;
    case Part::CONJ:
      return 11;
    case Part::INTERJ:
      return 12;
    case Part::NONE:
      return 13;
  }
  return 13;
}

template <typename T> [[nodiscard]] constexpr unsigned sortValue(T value) noexcept {
  if constexpr (std::is_enum_v<T>)
    return std::to_underlying(value);
  else
    return value.value;
}

[[nodiscard]] inline auto sortFields(const Analysis& q) {
  using enum Part;
  const unsigned none = 0;
  switch (q.part) {
    case N:
    case PRON:
    case PACK:
    case VPAR:
    case SUPINE:
      return std::tuple{unsigned{q.which.value},
                        sortValue(q.variant),
                        sortValue(q.number),
                        sortValue(q.caseOf),
                        sortValue(q.gender),
                        none,
                        none};
    case ADJ:
      return std::tuple{unsigned{q.which.value},
                        sortValue(q.variant),
                        sortValue(q.number),
                        sortValue(q.caseOf),
                        sortValue(q.gender),
                        sortValue(q.comparison),
                        none};
    case NUM:
      return std::tuple{unsigned{q.which.value},
                        sortValue(q.variant),
                        sortValue(q.number),
                        sortValue(q.caseOf),
                        sortValue(q.gender),
                        sortValue(q.numeralSort),
                        none};
    case V:
      return std::tuple{unsigned{q.which.value}, sortValue(q.variant), sortValue(q.number), sortValue(q.tense),
                        sortValue(q.voice),      sortValue(q.mood),    sortValue(q.person)};
    case ADV:
      return std::tuple{sortValue(q.comparison), none, none, none, none, none, none};
    case PREP:
      return std::tuple{sortValue(q.caseOf), none, none, none, none, none, none};
    default:
      return std::tuple{none, none, none, none, none, none, none};
  }
}

// INFO: Inflections_Package."<" on Quality_Record.
[[nodiscard]] inline bool inflectionsLess(const Analysis& left, const Analysis& right) {
  return left.part != right.part ? pofsPos(left.part) < pofsPos(right.part) : sortFields(left) < sortFields(right);
}

// NOTE: The order WORDS lists one record's readings in. A qu- pronoun orders by its variant alone, as the left one's
//  declension decides.
[[nodiscard]] inline bool listedBefore(const Analysis& left, std::size_t leftEnding, const Analysis& right,
                                       std::size_t rightEnding) {
  const bool quPronouns = left.part == Part::PRON && left.which.value == 1 && right.part == Part::PRON;
  if (quPronouns ? left.variant != right.variant : !(left == right))
    return quPronouns ? left.variant.value < right.variant.value : inflectionsLess(left, right);
  if (leftEnding != rightEnding)
    return leftEnding < rightEnding;
  return inflectionsLess(left, right);
}

// INFO: Reduce_Stem_List's "<=" on stem keys and its key-0 exception; the asymmetry is Whitaker's.
[[nodiscard]] constexpr bool stemKeyAdmits(unsigned char dictKey, unsigned char wanted, Part part) noexcept {
  return dictKey == wanted || wanted == 0 ||
         (dictKey == 0 && wanted >= 1 && wanted <= 2 && (part == Part::N || part == Part::ADJ || part == Part::V));
}

// INFO: Whitaker's matching predicates, dictionary on the left, inflection on
//  the right.
// INFO: Inflections_Package's "<=" on Decn_Record, which holds a declension or a conjugation.
[[nodiscard]] constexpr bool numberMatches(unsigned char left, unsigned char leftVariant, unsigned char right,
                                           unsigned char rightVariant) noexcept {
  return (right == left && rightVariant == leftVariant) || (right == 0 && rightVariant == 0 && left != 9) ||
         (right == left && rightVariant == 0);
}

[[nodiscard]] constexpr bool declensionMatches(Declension left, Variant leftVariant, Declension right,
                                               Variant rightVariant) noexcept {
  return numberMatches(left.value, leftVariant.value, right.value, rightVariant.value);
}

[[nodiscard]] constexpr bool conjugationMatches(Conjugation left, Variant leftVariant, Conjugation right,
                                                Variant rightVariant) noexcept {
  return numberMatches(left.value, leftVariant.value, right.value, rightVariant.value);
}

// INFO: Reduce_Stem_List's own "<=" on genders.
[[nodiscard]] constexpr bool genderMatches(Gender left, Gender right) noexcept {
  return right == left || right == Gender::X || (right == Gender::C && left != Gender::N);
}

// INFO: Reduce_Stem_List's ADJ comparison check.
[[nodiscard]] constexpr bool comparisonMatches(Comparison left, Comparison right) noexcept {
  return right == left || right == Comparison::X || left == Comparison::X;
}

// INFO: Load_Addons: a TACKON whose entry is a PACK of declension 1 or 2
//  and whose gloss starts "PACKON w/" is a PACKON.
template <typename Gloss> [[nodiscard]] constexpr bool isPackon(Declension declension, const Gloss& gloss) noexcept {
  return (declension.value == 1 || declension.value == 2) && gloss.starts_with("PACKON w/");
}

// INFO: Order_Parse_Array's rarity pass.
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

// INFO: Allowed_Stem's mood and tense ranges.
[[nodiscard]] constexpr bool moodIndToInf(Mood mood) noexcept {
  return mood == Mood::IND || mood == Mood::SUB || mood == Mood::IMP || mood == Mood::INF;
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

// INFO: An inflection row's share of Allowed_Stem; the entry's share is its verb kind and the stem the reading prints.
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
  [[nodiscard]] constexpr bool has(Bit bit) const noexcept { return (bits & bit) != 0; }
  constexpr void add(Bit bit) noexcept { bits |= bit; }
};

[[nodiscard]] constexpr AllowSet allowOf(Tense tense, Voice voice, Mood mood, Person person, Number number,
                                         bool noEnding) noexcept {
  AllowSet allow{};
  allow.add(AllowSet::IsVerb);
  if (tense == Tense::PRES && voice == Voice::ACTIVE && mood == Mood::IMP && person.value == 2 && number == Number::S &&
      noEnding)
    allow.add(AllowSet::ShortImp);
  if (mood == Mood::IMP && !((tense == Tense::PRES && person.value == 2) ||
                             (tense == Tense::FUT && (person.value == 2 || person.value == 3))))
    allow.add(AllowSet::ImpNoPerson);
  if (person.value != 3)
    allow.add(AllowSet::NotThird);
  if (voice == Voice::ACTIVE && mood == Mood::INF && tense == Tense::FUT)
    allow.add(AllowSet::DepKeep);
  else if (voice == Voice::ACTIVE && moodIndToInf(mood))
    allow.add(AllowSet::DepDrop);
  if (moodIndToImp(mood) &&
      ((voice == Voice::PASSIVE && tensePresToFut(tense)) || (voice == Voice::ACTIVE && tensePerfToFutp(tense))))
    allow.add(AllowSet::SemidepDrop);
  return allow;
}

// INFO: dic/duc/fac/fer shortened imperative (G&L 130.5).
template <typename Stem>
[[nodiscard]] constexpr bool allowedStem(AllowSet allow, VerbKind kind, bool conjugationThreeOne,
                                         const Stem& stem) noexcept {
  if (!allow.has(AllowSet::IsVerb))
    return true;
  bool allowed = true;
  if (allow.has(AllowSet::ShortImp) && conjugationThreeOne)
    allowed = stem.ends_with("dic") || stem.ends_with("duc") || stem.ends_with("fac") || stem.ends_with("fer");
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

} // namespace latin
