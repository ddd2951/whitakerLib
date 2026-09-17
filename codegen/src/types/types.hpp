#pragma once

#include "util/reflect_util.hpp"

#include <string_view>
#include <type_traits>

enum class TypeNumber : char {
  X = 'X',
  S = 'S',
  P = 'P',
  NONE = '0',
};

enum class TypeTense : int {
  X,
  PRES,
  IMPF,
  FUT,
  PERF,
  PLUP,
  FUTP,
  NONE,
};

enum class TypeVoice : int {
  X,
  ACTIVE,
  PASSIVE,
  NONE,
};

enum class TypeMood : int {
  X,
  IND,
  SUB,
  IMP,
  PPL,
  INF,
  NONE,
};

enum class PosTokenTypes {
  N,
  PRON,
  V,
  ADJ,
  ADV,
  PREP,
  NUM,
  CONJ,
  INTERJ,
  PACK,
  SUPINE,
  VPAR,
  NONE,
  // NOTE: X is ADDONS' "any part"; it sits after NONE so the ordinals the
  //  image asserts hold, and maps to rel::Part::Any.
  X
};

enum class TypeArea : char {
  X = 'X',
  A = 'A',
  B = 'B',
  D = 'D',
  E = 'E',
  G = 'G',
  L = 'L',
  M = 'M',
  P = 'P',
  S = 'S',
  T = 'T',
  W = 'W',
  Y = 'Y',
  NONE = '0'
};

enum class TypeAge : char {
  X = 'X',
  A = 'A',
  B = 'B',
  C = 'C',
  D = 'D',
  E = 'E',
  F = 'F',
  G = 'G',
  H = 'H',
  NONE = '0'
};

enum class TypeFrequency : char {
  X = 'X',
  A = 'A',
  B = 'B',
  C = 'C',
  D = 'D',
  E = 'E',
  F = 'F',
  I = 'I',
  M = 'M',
  N = 'N',
  NONE = '0'
};

enum class TypeGeography : char {
  X = 'X',
  A = 'A',
  B = 'B',
  C = 'C',
  D = 'D',
  E = 'E',
  F = 'F',
  G = 'G',
  H = 'H',
  I = 'I',
  J = 'J',
  K = 'K',
  N = 'N',
  P = 'P',
  Q = 'Q',
  R = 'R',
  S = 'S',
  U = 'U',
  NONE = '0'
};

enum class TypeSource : char {
  X = 'X',
  A = 'A',
  B = 'B',
  C = 'C',
  D = 'D',
  E = 'E',
  F = 'F',
  G = 'G',
  H = 'H',
  I = 'I',
  J = 'J',
  K = 'K',
  L = 'L',
  M = 'M',
  N = 'N',
  O = 'O',
  P = 'P',
  Q = 'Q',
  R = 'R',
  S = 'S',
  T = 'T',
  U = 'U',
  V = 'V',
  W = 'W',
  Y = 'Y',
  Z = 'Z',
  NONE = '0'
};

enum class TypeGender : char {
  X = 'X',
  M = 'M',
  F = 'F',
  N = 'N',
  C = 'C',
  NONE = '0',
};

enum class TypeNounKind : char {
  X = 'X',
  S = 'S',
  M = 'M',
  A = 'A',
  G = 'G',
  N = 'N',
  P = 'P',
  T = 'T',
  L = 'L',
  W = 'W',
  // NOTE: At tokenizer we treat lower and uppercase as two different types, left
  // to emitter to decide if they are treated different
  p = 'p',
  t = 't',
  w = 'w',
  x = 'x',
  NONE = '0',
};

enum class TypeCase : int { X, NOM, GEN, DAT, ACC, ABL, VOC, LOC, NONE };

enum class TypeComparison : int { X, POS, COMP, SUPER, NONE };

// INFO: TO_BE is used only by the synthesized esse entry.
enum class TypeVerbKind : int {
  X,
  TO_BE,
  TO_BEING,
  GEN,
  DAT,
  ABL,
  TRANS,
  INTRANS,
  IMPERS,
  DEP,
  SEMIDEP,
  PERFDEF,
  NONE
};

enum class TypePronounKind : int {
  X,
  PERS,
  REL,
  REFLEX,
  DEMONS,
  INTERR,
  INDEF,
  ADJECT,
  NONE,
};

enum class TypePackonKind : int {
  X,
  INTERR,
  REL,
  INDEF,
  ADJECT,
  NONE,
};

enum class TypeNumeralSort : int {
  X,
  CARD,
  ORD,
  DIST,
  ADVERB,
  NONE,
};

template <typename E>
  requires std::is_enum_v<E>
[[nodiscard]] constexpr std::string_view toName(E value) noexcept {
  const std::string_view name = util::enumToSv(value);
  return name.empty() ? "?" : name;
}
