#include <cstddef>
#include <type_traits>
#include <variant>

#include "expand/common/reading.hpp"
#include "expand/common/rows.hpp"
#include "expand/common/tables.hpp"
#include "word/word.hpp"

namespace {

using namespace expand;

Reading fromJoined(const Joined& j) {
  const DictlineEntry d = entry(j.entry);
  const InflectsEntry f = inflection(j.inflection);
  return Reading{stemAt(d, j.column),
                 f.stemKey,
                 d.grammar,
                 f.grammar,
                 d.age,
                 d.area,
                 d.geography,
                 d.frequency,
                 d.source,
                 f.age,
                 f.frequency,
                 d.senses,
                 false};
}

Reading fromUnique(UniquesRow row) {
  const word::Uniques w{source::UniquesIndex{row.value}};
  return Reading{w.form(),
                 latin::StemKey{1},
                 w.grammar(),
                 w.inflection(),
                 w.age(),
                 w.area(),
                 w.geography(),
                 w.frequency(),
                 w.source(),
                 latin::Age::X,
                 latin::Frequency::X,
                 w.senses(),
                 true};
}

Reading fromUniqueAdverb(const UniqueAdverb& a) {
  Reading reading = fromUnique(a.entry);
  const InflectsEntry f = inflection(a.inflection);
  reading.key = f.stemKey;
  reading.inflection = f.grammar;
  reading.inflectionAge = f.age;
  reading.inflectionFrequency = f.frequency;
  return reading;
}

} // namespace

expand::Reading expand::readingOf(const Origin& origin) {
  return std::visit(
      [&](const auto& o) -> Reading {
        using O = std::remove_cvref_t<decltype(o)>;
        if constexpr (std::is_same_v<O, Joined>)
          return fromJoined(o);
        else if constexpr (std::is_same_v<O, Unique>)
          return fromUnique(o.entry);
        else
          return fromUniqueAdverb(o);
      },
      origin);
}
