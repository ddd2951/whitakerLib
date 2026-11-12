#include "execution_steps/steps.hpp"

#include "emitter/relationship_emitter/rendered_analysis.hpp"
#include "storage/result.hpp"
#include "storage/back.hpp"
#include "storage/stems.hpp"
#include "storage/lookup_metadata.hpp"
#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <utility>

namespace rel = whitaker::relationship;

void execution_steps::lookup_tables::packLookupStems() {
  using storage::back;
  using storage::out;
  using storage::result;
  using storage::stems;
  result.back = back;
  unsigned maximumPart = 0, affixInversions = 0, plainAfterAffix = 0;
  std::string previousText{};
  bool affixOnly = false;
  std::map<std::pair<std::string, unsigned>, unsigned> previousKey{};
  for (const auto& s : stems) {
    const auto text = emitter::normalize(s.text);
    if (text != previousText) {
      previousText = text;
      affixOnly = false;
    }
    if (s.klass == rel::kNoLookupClass)
      affixOnly = true;
    else if (affixOnly)
      ++plainAfterAffix;
    if (s.stemKey != 255) {
      const auto [it, fresh] = previousKey.emplace(std::pair{emitter::normalize(s.text), s.dictionary}, s.stemKey);
      if (!fresh && s.stemKey < it->second)
        ++affixInversions;
      it->second = s.stemKey;
    }
    maximumPart = std::max(maximumPart, unsigned(std::to_underlying(s.part)));
    result.stemTexts.push_back(emitter::normalize(s.text));
    result.stems.push_back({static_cast<std::uint16_t>(s.dictionary), static_cast<std::uint8_t>(s.stemKey),
                            static_cast<std::uint8_t>(std::to_underlying(s.part) | (s.column << 4) | (s.flags << 6)),
                            static_cast<std::uint16_t>(s.klass)});
  }
  out << "static_assert(" << maximumPart << " < 16);\n";
  out << "static_assert(" << plainAfterAffix << " == 0, \"plain stems must precede affix-only members\");\n";

  out << "static_assert(" << affixInversions << " == 0, \"shared stems changed affix order within an entry\");\n";
}
