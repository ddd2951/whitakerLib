#include "execution_steps/steps.hpp"

#include "expand/common/reading.hpp"
#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "search/two_ended_schema.hpp"
#include "storage/by_stem.hpp"
#include "storage/classes.hpp"
#include "storage/rows.hpp"
#include "storage/stems.hpp"
#include "word/word.hpp"
#include <algorithm>
#include <map>
#include <utility>
#include <variant>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::buildLookupClasses() {
  using storage::byStem;
  using storage::classes;
  using storage::ClassKey;
  using storage::rows;
  using storage::stems;
  namespace te = whitaker::two_ended;
  std::map<ClassKey, unsigned> classIds{};
  for (const auto& [address, ids] : byStem) {
    const auto [dictionary, column] = address;
    const bool unique = dictionary >= word::kUniqueBase;
    const auto r =
        expand::readingOf(unique ? expand::Origin{expand::Unique{word::UniquesRow{dictionary - word::kUniqueBase}}}
                                 : expand::Origin{expand::Joined{word::DictlineRow{dictionary}, word::InflectsRow{0},
                                                                 latin::StemKey{static_cast<unsigned char>(column)}}});
    // INFO: Empty + empty can only spell the rejected empty input. Empty stems with real endings stay.
    if (r.stem.empty() && std::ranges::all_of(ids, [&](const unsigned id) { return rows[id].suffix.empty(); }))
      continue;
    bool qu = false, conj31 = false;
    latin::VerbKind kind{};
    if (const auto* const p = std::get_if<latin::Pronoun>(&r.entry))
      qu = p->declension.value == 1;
    if (const auto* const v = std::get_if<latin::Verb>(&r.entry)) {
      kind = v->verbKind;
      conj31 = v->conjugation.value == 3 && v->conjugationVariant.value == 1;
    }
    std::vector<bool> keep{};
    for (const auto id : ids) {
      const auto g = rel::unpackGrammar(rows[id].grammar).analysis;
      keep.push_back(
          unique || g.part != latin::Part::verb ||
          latin::allowedStem(latin::allowOf(g.tense, g.voice, g.mood, g.person, g.number, rows[id].ending.empty()),
                             kind, conj31, r.stem));
    }
    ClassKey key{ids, keep};
    const auto [it, fresh] = classIds.try_emplace(key, static_cast<unsigned>(classes.size()));
    if (fresh)
      classes.push_back(std::move(key));
    stems.push_back(
        {std::string(r.stem), dictionary, column - 1, (unique ? te::kUnique : 0) | (qu ? te::kQu : 0), it->second});
  }
}
