#include "rows.hpp"

#include "expand/common/tables.hpp"
#include "word/word.hpp"

expand::DictlineEntry expand::entry(DictlineRow row) {
  const word::Dictline w{row};
  return {w.stem1(),     w.stem2(),  w.stem3(), w.stem4(),
          w.grammar(),   w.age(),    w.area(),  w.geography(),
          w.frequency(), w.source(), w.senses()};
}

expand::InflectsEntry expand::inflection(InflectsRow row) {
  const word::Inflects w{row};
  return {w.grammar(), w.stemKey(), w.characterCount(),
          w.ending(),  w.age(),     w.frequency()};
}
