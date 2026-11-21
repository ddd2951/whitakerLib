// INFO: Capacity from source forms and licensed add-on rows, including readings the rarity sweep drops.
#include "execution_steps/run.hpp"
#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "expand/expand.hpp"
#include "expand/common/stem_column.hpp"
#include "source/source.hpp"
#include "storage/image.hpp"
#include "word/word.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

namespace rel = whitaker::relationship;
using Labels = std::pair<latin::Age, latin::Frequency>;
using Endings = std::map<std::string, std::set<std::tuple<unsigned, latin::Age, latin::Frequency>>>;

// INFO: Every ending + fix in one answer is a suffix of that query. Sum along each reverse-trie path, ignoring
//  stem licensing, rarity and first-answer selection: each can only lower this bound.
static unsigned suffixBound(const Endings& endings) {
  unsigned maximum = 0;
  for (const auto& [word, unused] : endings) {
    (void)unused;
    unsigned sum = 0;
    for (std::size_t i = 0; i <= word.size(); ++i)
      if (const auto found = endings.find(word.substr(i)); found != endings.end())
        sum += static_cast<unsigned>(found->second.size());
    maximum = std::max(maximum, sum);
  }
  return maximum;
}

int main(const int argc, char** const argv) {
  if (argc != 2)
    return 2;
  runner::run<^^execution_steps::dictline, ^^execution_steps::inflects, ^^execution_steps::uniques,
              ^^execution_steps::addons, ^^execution_steps::candidates, ^^execution_steps::expansion>();
  // NOTE: Unswept forms include Try_Tackons' restored rows. Entry ids and grammar do not distinguish changes.
  using Key = std::tuple<std::string, std::string, latin::Age, latin::Frequency, bool>;
  std::map<std::string, std::set<Key>> dictionary{};
  for (std::size_t i = 0; i < expand::resultCount(); ++i) {
    const expand::ResultIndex index{i};
    const auto r = expand::reading(index);
    const auto g = emitter::analysisOf(r.entry, r.inflection, r.key);
    const auto spelling = expand::spelling(index);
    if (spelling.empty() || spelling.size() > facts::kMaxWordCharacters)
      continue;
    dictionary[emitter::normalize(spelling)].emplace(
        std::string(r.stem), std::string(spelling.substr(r.stem.size())), r.inflectionAge, r.inflectionFrequency,
        g.part == latin::Part::adverb && latin::partOf(r.entry) == latin::Part::adjective);
  }
  unsigned plain = 0;
  for (const auto& [word, keys] : dictionary) {
    (void)word;
    plain = std::max(plain, static_cast<unsigned>(keys.size()));
  }
  runner::run<^^execution_steps::image_data, ^^execution_steps::lookup_tables, ^^execution_steps::finish_image_data>();
  const auto& data = storage::image;
  if (data.trimmedReadings.size() > 256) {
    std::cerr << "Try_Tackons' scratch flags no longer fit\n";
    return 1;
  }
  // INFO: The compact affix path recovers physical columns from entry declarations and stored keys. Prove that
  //  recovery against the source's columnForKey, including the shared first/second and declared-degree cases.
  for (const auto& stem : data.stems) {
    if (stem.stemKey == 255)
      continue;
    const auto& entry = data.entries[stem.dictionary];
    unsigned column = stem.stemKey ? unsigned(stem.stemKey) - 1 : 0;
    if ((entry.part == latin::Part::adjective || entry.part == latin::Part::adverb ||
         entry.part == latin::Part::numeral) &&
        (entry.comparison == latin::Comparison::comparative || entry.comparison == latin::Comparison::superlative ||
         (entry.part == latin::Part::numeral && entry.numeralSort != latin::NumeralSort::any)))
      column = 0;
    const auto key = latin::StemKey{static_cast<unsigned char>(stem.stemKey ? stem.stemKey : 1)};
    const auto expected = expand::internal::columnForKey(word::entry(word::DictlineRow{stem.dictionary}), key);
    if (expected.value != column + 1 || rel::column(stem) != column) {
      std::cerr << "affix stem column no longer matches the source at entry " << stem.dictionary << '\n';
      return 1;
    }
  }
  const auto text = [&](const std::uint32_t at) { return std::string(data.strings.blob().c_str() + at); };
  Endings packon{}, suffix{}, prefix{};
  unsigned packonNumber = 0;
  for (unsigned id = 0; id < data.addons.size(); ++id) {
    const auto& addon = data.addons[id];
    if (addon.kind != latin::AddonKind::packon && addon.kind != latin::AddonKind::suffix)
      continue;
    std::vector<rel::Run> runs{};
    if (addon.kind == latin::AddonKind::packon) {
      ++packonNumber;
      for (const auto& c : data.classes)
        if (((c.rules >> rel::kRulePackonShift) & rel::kRulePackonMask) == packonNumber)
          runs.push_back(rel::rows(c));
    } else
      runs.push_back(rel::rows(addon));
    for (const auto run : runs)
      for (auto i = run.first; i < rel::end(run); ++i) {
        const auto& row = data.classRows[i];
        const auto& inflection = data.inflections[row.inflect];
        (addon.kind == latin::AddonKind::packon
             ? packon
             : suffix)[(addon.kind == latin::AddonKind::packon ? text(inflection.ending) + text(addon.fix)
                                                               : text(addon.fix) + text(inflection.ending))]
            .emplace(id, inflection.age, inflection.frequency);
      }
  }
  for (const auto& inflection : data.inflections)
    prefix[text(inflection.ending)].emplace(0, inflection.age, inflection.frequency);
  const auto p = suffixBound(packon), s = suffixBound(suffix), q = suffixBound(prefix);
  std::ostringstream out{};
  out << "// Generated from source forms and add-on rows; do not edit.\n#pragma once\n#include <algorithm>\n"
         "namespace whitaker::parse {\n"
      << "inline constexpr unsigned kDictionaryChanges = " << plain << ";\n"
      << "inline constexpr unsigned kPackonChanges = " << p << ";\n"
      << "inline constexpr unsigned kSuffixChanges = " << s << ";\n"
      << "inline constexpr unsigned kPrefixChanges = " << q
      << ";\n"
         "// INFO: One dictionary call, a tickon call or packon fallback, and the whole-word packon call.\n"
         "inline constexpr unsigned kWordChanges = 2 * kDictionaryChanges + 2 * kPackonChanges;\n"
         "// INFO: Before enclitics: word + two dictionary calls (prefix variant/syncope), plus Roman.\n"
         "//  Enclitic: word + two dictionary calls. Recursive tackons retain one accepted branch.\n"
         "//  Affixes or respelling run only after those answers are empty. Adverbs can double each group.\n"
         "inline constexpr unsigned kMaximumChanges = 2 * std::max({2 * kWordChanges + 4 * kDictionaryChanges + 1,\n"
         "  kSuffixChanges, kPrefixChanges, kWordChanges + kDictionaryChanges});\n"
         "static_assert(kMaximumChanges + 1 <= 255, \"word-local change index no longer fits\");\n"
         "} // namespace whitaker::parse\n";
  const auto generated = out.str();
  constexpr const char* path = "src/parse/change_capacity.hpp";
  if (std::string_view(argv[1]) == "--write") {
    std::ofstream file(path);
    file << generated;
    if (!file)
      return 2;
  } else {
    std::ifstream file(path);
    const std::string written{std::istreambuf_iterator<char>(file), {}};
    if (written != generated) {
      std::cerr << path << " is stale: build write_reading_capacity\n";
      return 1;
    }
  }
  std::cout << "change bounds: dictionary " << plain << ", packon " << p << ", suffix " << s << ", prefix " << q
            << '\n';
}
