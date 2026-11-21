#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "expand/common/stem_column.hpp"
#include "src/search/relationship_image.hpp"
#include "word/word.hpp"

#include <whitaker.hpp>

#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace rel = whitaker::relationship;

namespace {

using Stems = std::vector<std::pair<unsigned, std::string>>;

[[nodiscard]] std::string shown(const Stems& stems) {
  std::string out{};
  for (const auto& [key, spelling] : stems)
    out += std::format(" {}:{}", key, spelling);
  return out;
}

void validateStems(const std::uint32_t index, const Stems& expected) {
  const auto found = rel::stems(static_cast<std::uint16_t>(index));
  Stems given{};
  for (const auto& stem : std::span{found.stems, found.count})
    given.emplace_back(stem.key, stem.spelling);
  if (given != expected)
    error::fatal(
        std::format("image: dictionary word {} has stems{}, its source{}", index, shown(given), shown(expected)));
}

// INFO: Each key's stem as the source spells it. esse's empty second stem is one; DICTLINE's blank ones are not.
[[nodiscard]] Stems dictlineStems(const std::uint32_t index) {
  const auto& entry = word::entry(word::DictlineRow{index});
  Stems out{};
  for (unsigned char key = 1; key <= 4; ++key) {
    const auto column = expand::internal::columnForKey(entry, latin::StemKey{key});
    const auto text = expand::stemAt(entry, column);
    if (text && (!text->empty() || index == word::kEsse.value))
      out.emplace_back(key, std::string{*text});
  }
  return out;
}

} // namespace

void execution_steps::image_file::checkDictionaryWordStems() {
  const std::uint32_t uniques = source::scheme::uniques::kEntriesPerFile;
  for (std::uint32_t index = 0; index < word::kDictlineWords; ++index)
    validateStems(index, dictlineStems(index));
  for (std::uint32_t index = 0; index < uniques; ++index)
    validateStems(word::kDictlineWords + index, {{0, std::string{word::entry(word::UniquesRow{index}).form}}});
}
