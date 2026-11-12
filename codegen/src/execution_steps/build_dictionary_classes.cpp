#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "error/error.hpp"
#include "helpers/addon_value.hpp"
#include "helpers/grammar_rules.hpp"
#include "storage/class_grammars.hpp"
#include "storage/image.hpp"
#include "storage/packons.hpp"
#include "word/word.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using namespace emitter;
using helpers::addonValue;
using helpers::grammarRules;

using GrammarKey = std::array<std::uint16_t, 4>;

// NOTE: Entries with the same key share one class, so the key holds only what the join reads. Verb kind goes on each
//  entry instead.
[[nodiscard]] GrammarKey grammarKey(const latin::Entry& grammar, const std::uint16_t packon) {
  const latin::Part part = partOf(grammar);
  GrammarKey key{static_cast<std::uint16_t>(part), 0, 0, packon};
  std::visit(
      [&key](const auto& value) {
        using Grammar = std::remove_cvref_t<decltype(value)>;
        if constexpr (std::is_same_v<Grammar, latin::Noun>)
          key = {key[0], addonValue(value.declension), addonValue(value.declensionVariant), addonValue(value.gender)};
        else if constexpr (std::is_same_v<Grammar, latin::Pronoun> || std::is_same_v<Grammar, latin::Packon>)
          key = {key[0], addonValue(value.declension), addonValue(value.declensionVariant), key[3]};
        else if constexpr (std::is_same_v<Grammar, latin::Verb>)
          key = {key[0], addonValue(value.conjugation), addonValue(value.conjugationVariant), 0};
        else if constexpr (std::is_same_v<Grammar, latin::Adjective>)
          key = {key[0], addonValue(value.declension), addonValue(value.declensionVariant),
                 addonValue(value.comparison)};
        else if constexpr (std::is_same_v<Grammar, latin::Numeral>)
          key = {key[0], addonValue(value.declension), addonValue(value.declensionVariant),
                 addonValue(value.numeralSort)};
        else if constexpr (std::is_same_v<Grammar, latin::Adverb>)
          key = {key[0], addonValue(value.comparison), 0, 0};
        else if constexpr (std::is_same_v<Grammar, latin::Preposition>)
          key = {key[0], addonValue(value.caseOf), 0, 0};
      },
      grammar);
  return key;
}

[[nodiscard]] latin::VerbKind entryVerbKind(const latin::Entry& grammar) {
  const auto* const verb = std::get_if<latin::Verb>(&grammar);
  return verb != nullptr ? verb->verbKind : latin::VerbKind::X;
}

[[nodiscard]] std::vector<latin::Entry> internClasses(const std::map<std::string, std::uint16_t>& packons,
                                                      ImageData& image) {
  std::map<GrammarKey, std::uint16_t> classIds{};
  std::vector<latin::Entry> classGrammars{};
  for (std::uint32_t index = 0; index < word::kDictlineWords; ++index) {
    const word::DictlineEntry& dictionary = word::entry(word::DictlineRow{index});
    std::uint16_t packon = 0;
    if (partOf(dictionary.grammar) == latin::Part::PACK) {
      const auto named = packons.find(normalize(dictionary.packon));
      if (named != packons.end())
        packon = named->second;
    }
    const auto [entry, inserted] =
        classIds.emplace(grammarKey(dictionary.grammar, packon), static_cast<std::uint16_t>(classGrammars.size()));
    if (inserted) {
      if (entry->second > rel::kClassIdMask)
        error::fatal("image: more classes than a dictionary's class id can name");
      classGrammars.push_back(dictionary.grammar);
      image.classes.push_back(rel::ClassRecord{.rules = grammarRules(dictionary.grammar, packon)});
    }
    // NOTE: Also for an entry that produces no form of its own, which only an addon reaches: a PACK, audo (V 8 2).
    image.dictionaries[index].meaning = image.strings.intern(dictionary.senses);
    image.dictionaries[index].age = dictionary.labels.age;
    image.dictionaries[index].frequency = dictionary.labels.frequency;
    image.dictionaries[index].classId =
        entry->second |
        static_cast<std::uint16_t>(std::to_underlying(entryVerbKind(dictionary.grammar)) << rel::kClassVerbKindShift);
  }
  return classGrammars;
}

} // namespace

void execution_steps::image_data::buildDictionaryClasses() {
  using storage::classGrammars;
  using storage::image;
  using storage::packons;
  classGrammars = internClasses(packons, image);
}
