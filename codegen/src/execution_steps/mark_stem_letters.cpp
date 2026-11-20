#include "execution_steps/steps.hpp"

#include "error/error.hpp"
#include "expand/common/stem_column.hpp"
#include "storage/image.hpp"
#include "word/word.hpp"

#include <cstdint>
#include <format>
#include <string_view>

namespace {

// INFO: What folding took from a stem's source spelling, for two_ended::unfold: bit k of the low byte, that its kth
//  u or i was a v or a j; the high byte, how many letters it starts with are capitals.
[[nodiscard]] std::uint16_t lettersOf(const std::string_view source) {
  unsigned folds = 0, capitals = 0, mask = 0;
  for (std::size_t i = 0; i < source.size(); ++i) {
    const char c = source[i];
    if (c >= 'A' && c <= 'Z') {
      if (capitals != i)
        error::fatal(std::format("image: stem {} has a capital after its first small letter", source));
      ++capitals;
    }
    const char small = static_cast<char>(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c);
    if (small != 'u' && small != 'v' && small != 'i' && small != 'j')
      continue;
    if (folds == 8)
      error::fatal(std::format("image: stem {} has more than 8 letters u, v, i or j", source));
    if (small == 'v' || small == 'j')
      mask |= 1u << folds;
    ++folds;
  }
  return static_cast<std::uint16_t>(capitals << 8 | mask);
}

} // namespace

void execution_steps::finish_image_data::markStemLetters() {
  using storage::image;
  image.stemLetters.clear();
  for (const auto& stem : image.stems) {
    if (stem.dictionary >= word::kUniqueBase) {
      image.stemLetters.push_back(lettersOf(word::entry(word::UniquesRow{stem.dictionary - word::kUniqueBase}).form));
      continue;
    }
    const auto column = latin::StemKey{static_cast<unsigned char>(whitaker::relationship::column(stem) + 1)};
    const auto text = expand::stemAt(word::entry(word::DictlineRow{stem.dictionary}), column);
    if (!text)
      error::fatal(std::format("image: dictionary word {} has no stem in its stem row's column", stem.dictionary));
    image.stemLetters.push_back(lettersOf(*text));
  }
}
