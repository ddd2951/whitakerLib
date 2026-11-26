#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "oracle/src/dictionary.hpp"
#include "readings/dictionary_form.hpp"
#include "src/search/dictionary_form.hpp"
#include "src/search/relationship_image.hpp"
#include "word/word.hpp"

#include <whitaker.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace rel = whitaker::relationship;

namespace {

constexpr std::uint32_t kDictionaryWords = word::kDictlineWords + source::scheme::uniques::kEntriesPerFile;

[[nodiscard]] lines::Record referenceRecord(const latin::Entry& grammar, std::array<std::string, 4> stems) {
  lines::Record record{.stems = std::move(stems)};
  record.part = latin::name(latin::partOf(grammar));
  std::visit(
      [&record](const auto& value) {
        if constexpr (requires { value.declension; }) {
          record.which = value.declension.value;
          record.variant = value.declensionVariant.value;
        } else if constexpr (requires { value.conjugation; }) {
          record.which = value.conjugation.value;
          record.variant = value.conjugationVariant.value;
        }
        if constexpr (requires { value.gender; })
          record.gender = latin::name(value.gender);
        if constexpr (requires { value.comparison; })
          record.comparison = latin::name(value.comparison);
        if constexpr (requires { value.verbKind; })
          record.kind = latin::name(value.verbKind);
        if constexpr (requires { value.numeralSort; })
          record.kind = latin::name(value.numeralSort);
        if constexpr (requires { value.caseOf; })
          record.object = latin::name(value.caseOf);
      },
      grammar);
  return record;
}

[[nodiscard]] lines::Record referenceWord(const std::uint32_t index) {
  if (index >= word::kUniqueBase) {
    const auto& word = word::entry(word::UniquesRow{index - word::kUniqueBase});
    return referenceRecord(word.grammar, {std::string{word.form}});
  }
  const auto& word = word::entry(word::DictlineRow{index});
  return referenceRecord(word.grammar, {std::string{word.stem1.value_or("")}, std::string{word.stem2.value_or("")},
                                        std::string{word.stem3.value_or("")}, std::string{word.stem4.value_or("")}});
}

// INFO: A preposition's case comes from a reading. Every row in its image class must give that same case.
[[nodiscard]] latin::Case prepositionCase(const std::uint16_t id) {
  if (rel::entry(id).part != latin::Part::preposition)
    return {};
  const auto rows = rel::rows(rel::dictionaryClass(rel::dictionary(id).classId & rel::kClassIdMask));
  if (rows.count == 0)
    error::fatal(std::format("image: preposition {} has no class rows", id));
  const auto governedCase = rel::grammar(rel::classRow(rows.first).description).caseOf;
  for (auto i = rows.first; i < rel::end(rows); ++i) {
    const auto grammar = rel::grammar(rel::classRow(i).description);
    if (grammar.part != latin::Part::preposition || grammar.caseOf != governedCase)
      error::fatal(std::format("image: preposition {} has inconsistent class rows", id));
  }
  return governedCase;
}

} // namespace

std::size_t readings::dictionaryFormMaximum() {
  std::size_t maximum{};
  for (std::uint32_t index = 0; index < kDictionaryWords; ++index)
    maximum = std::max(maximum, lines::dictionaryForm(referenceWord(index)).size());
  return maximum;
}

void execution_steps::image_file::checkDictionaryForms() {
  if (rel::section(rel::Section::Dictionaries).count != kDictionaryWords)
    error::fatal("image: dictionary form check has a different count of dictionary words");
  for (std::uint32_t index = 0; index < kDictionaryWords; ++index) {
    const auto reference = referenceWord(index);
    for (const auto& stem : reference.stems) {
      if (stem.size() > rel::kMaxDictionaryFormPartLength)
        error::fatal(std::format("dictionary word {} has a {}-byte stem, longer than a dictionary form part's {} bytes",
                                 index, stem.size(), rel::kMaxDictionaryFormPartLength));
    }
    const auto expected = lines::dictionaryForm(reference);
    if (expected.size() > whitaker::kMaxDictionaryFormLength)
      error::fatal(
          std::format("dictionary word {} needs {} bytes for its dictionary form: build write_reading_capacity", index,
                      expected.size()));
    const auto id = static_cast<std::uint16_t>(index);
    const auto form = rel::dictionaryForm(id, prepositionCase(id));
    const std::string_view found{form.text};
    if (found != expected)
      error::fatal(
          std::format("image: dictionary word {} has dictionary form <{}>, WORDS gives <{}>", index, found, expected));
  }
}
