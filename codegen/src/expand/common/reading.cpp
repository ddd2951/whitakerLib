#include <variant>

#include "expand/common/reading.hpp"
#include "expand/common/stem_column.hpp"
#include "word/word.hpp"

namespace {

using namespace expand;

Reading from(const Joined& j) {
  const word::DictlineEntry& d = word::entry(j.entry);
  const word::InflectsEntry& f = word::entry(j.inflection);
  return Reading{.stem = *stemAt(d, j.column),
                 .key = f.stemKey,
                 .entry = d.grammar,
                 .inflection = f.grammar,
                 .entryAge = d.labels.age,
                 .entryFrequency = d.labels.frequency,
                 .inflectionAge = f.age,
                 .inflectionFrequency = f.frequency,
                 .senses = d.senses,
                 .unique = false};
}

Reading from(const Unique& unique) {
  const word::UniquesEntry& u = word::entry(unique.entry);
  return Reading{.stem = u.form,
                 .key = latin::StemKey{1},
                 .entry = u.grammar,
                 .inflection = u.inflection,
                 .entryAge = u.labels.age,
                 .entryFrequency = u.labels.frequency,
                 .inflectionAge = latin::Age::X,
                 .inflectionFrequency = latin::Frequency::X,
                 .senses = u.senses,
                 .unique = true};
}

} // namespace

expand::Reading expand::readingOf(const Origin& origin) {
  return std::visit([](const auto& o) { return from(o); }, origin);
}
