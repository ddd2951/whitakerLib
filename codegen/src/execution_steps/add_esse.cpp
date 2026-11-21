#include "execution_steps/steps.hpp"

#include "storage/dictline.hpp"

void execution_steps::dictline::addEsse() {
  storage::dictline[word::kEsse.value] = {
      .stem1 = "s",
      .stem2 = "",
      .stem3 = "fu",
      .stem4 = "fut",
      .grammar = latin::Verb{latin::Conjugation{5}, latin::Variant{1}, latin::VerbKind::toBe},
      .labels = {.age = latin::Age::X,
                 .area = latin::Area::X,
                 .geography = latin::Geography::X,
                 .frequency = latin::Frequency::A,
                 .source = latin::Source::X},
      .senses = "be; exist; (also used to form verb perfect passive tenses) with NOM PERF PPL",
      .packon = {},
  };
}
