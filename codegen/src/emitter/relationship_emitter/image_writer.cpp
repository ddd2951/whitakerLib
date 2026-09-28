#include "image_writer.hpp"

#include "emitter/image_file.hpp"
#include "src/search/relationship_schema.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace rel = whitaker::relationship;

namespace emitter {

[[nodiscard]] ImageFile encodeImage(const ImageData& imageData,
                                    const DirectOutputMachine& machine) {
  ImageFile image;
  const Saved header = image.reserve("header", {1, rel::kHeaderBytes * 8});
  const Saved directory = image.reserve(
      "directory", {rel::kSectionCount, rel::kDirectoryBytes * 8});
  std::array<rel::DirectoryEntry, rel::kSectionCount> entries{};

  const auto saveSection =
      [&]<typename Save>(rel::Section id, std::string_view name, Shape shape,
                         std::uint64_t directoryCount, Save&& save) {
        const Saved saved = image.save(name, shape, static_cast<Save&&>(save));
        entries[static_cast<std::size_t>(id)] = {
            saved.byteStart, saved.byteLength(), directoryCount};
      };

  saveSection(rel::Section::States, "states",
              {machine.states(), rel::kStateBytes * 8}, machine.states(),
              [&](Encoder& out) {
                for (std::size_t state = 0; state < machine.states(); ++state) {
                  const std::uint64_t packed = machine.packedStates()[state];
                  out.u32(static_cast<std::uint32_t>(packed));
                  out.u32(static_cast<std::uint32_t>(packed >> 32));
                  out.u8(machine.resultCounts()[state]);
                  out.u8(0);
                  out.u16(0);
                }
              });
  saveSection(rel::Section::Transitions, "transitions",
              {machine.transitions(), rel::kTransitionBytes * 8},
              machine.transitions(), [&](Encoder& out) {
                for (const std::uint64_t edge : machine.packedEdges())
                  out.u64(edge);
              });
  saveSection(rel::Section::Lexemes, "lexemes",
              {imageData.program.lexemes.size(), rel::kLexemeBytes * 8},
              imageData.program.lexemes.size(), [&](Encoder& out) {
                for (const Lexeme& lexeme : imageData.program.lexemes) {
                  out.u16(lexeme.dictionary);
                  out.u8(lexeme.stem);
                  out.u8(0);
                  out.u16(lexeme.paradigm);
                  out.u16(0);
                }
              });
  saveSection(rel::Section::Paradigms, "paradigms",
              {imageData.program.paradigms.size(), rel::kParadigmBytes * 8},
              imageData.program.paradigms.size(), [&](Encoder& out) {
                for (const Paradigm& paradigm : imageData.program.paradigms) {
                  out.u32(paradigm.firstTarget);
                  out.u8(paradigm.count);
                  out.u8(0);
                  out.u16(0);
                }
              });
  saveSection(rel::Section::Targets, "targets",
              {imageData.program.targets.size(), rel::kTargetBytes * 8},
              imageData.program.targets.size(), [&](Encoder& out) {
                for (const std::uint16_t target : imageData.program.targets)
                  out.u16(target);
              });
  // NOTE: Directory count is ordered analyses, not bytes; the reader checks it.
  saveSection(rel::Section::Instructions, "instructions",
              {imageData.program.instructions.size(), 8},
              imageData.analyses.size(), [&](Encoder& out) {
                for (const std::byte instruction :
                     imageData.program.instructions)
                  out.u8(std::to_integer<std::uint8_t>(instruction));
              });
  saveSection(rel::Section::Dictionaries, "dictionaries",
              {imageData.dictionaries.size(), rel::kDictionaryBytes * 8},
              imageData.dictionaries.size(), [&](Encoder& out) {
                for (const DictionaryRecord& dictionary :
                     imageData.dictionaries) {
                  for (const std::uint32_t orth : dictionary.orth)
                    out.u32(orth);
                  out.u32(dictionary.meaning);
                  out.u8(dictionary.age);
                  out.u8(dictionary.frequency);
                  out.u16(dictionary.classId);
                }
              });
  saveSection(rel::Section::Addons, "addons",
              {imageData.addons.size(), rel::kAddonBytes * 8},
              imageData.addons.size(), [&](Encoder& out) {
                for (const AddonRecord& addon : imageData.addons) {
                  out.u32(addon.fix);
                  out.u32(addon.meaning);
                  for (const std::uint16_t field : addon.target)
                    out.u16(field);
                  out.u8(std::to_underlying(addon.kind));
                  out.u8(std::to_underlying(addon.root));
                  out.u8(std::to_underlying(addon.targetPart));
                  out.u8(addon.rootKey);
                  out.u8(addon.targetKey);
                  out.u8(addon.connect);
                  out.u8(addon.targetGate);
                  out.u32(addon.rowStart);
                  out.u16(addon.rowCount);
                  out.u8(addon.firstRaw);
                  out.u16(0);
                }
              });
  saveSection(rel::Section::Classes, "classes",
              {imageData.classes.size(), rel::kClassBytes * 8},
              imageData.classes.size(), [&](Encoder& out) {
                for (const ClassRecord& record : imageData.classes) {
                  out.u32(record.rowStart);
                  out.u16(record.rowCount);
                  out.u16(record.gate);
                }
              });
  saveSection(rel::Section::FallbackRows, "fallback rows",
              {imageData.fallbackRows.size(), rel::kFallbackRowBytes * 8},
              imageData.fallbackRows.size(), [&](Encoder& out) {
                for (const FallbackRowRecord& row : imageData.fallbackRows) {
                  out.u16(row.inflect);
                  out.u16(row.description);
                }
              });
  saveSection(rel::Section::Inflections, "inflections",
              {imageData.inflections.size(), rel::kInflectionBytes * 8},
              imageData.inflections.size(), [&](Encoder& out) {
                for (const InflectionRecord& inflection :
                     imageData.inflections) {
                  out.u32(inflection.ending);
                  out.u8(inflection.key);
                  out.u8(inflection.allow);
                  out.u8(inflection.age);
                  out.u8(inflection.frequency);
                }
              });
  saveSection(rel::Section::Endings, "endings",
              {imageData.endings.size(), rel::kEndingBytes * 8},
              imageData.endings.size(), [&](Encoder& out) {
                for (const EndingRecord& ending : imageData.endings) {
                  out.u32(ending.text);
                  out.u16(ending.first);
                  out.u16(ending.count);
                }
              });
  saveSection(rel::Section::FallbackStems, "fallback stems",
              {imageData.fallbackStems.size(), rel::kFallbackStemBytes * 8},
              imageData.fallbackStems.size(), [&](Encoder& out) {
                for (const FallbackStemRecord& stem : imageData.fallbackStems) {
                  out.u32(stem.text);
                  out.u16(stem.dictionary);
                  out.u8(stem.key);
                  out.u8(std::to_underlying(stem.part));
                }
              });
  saveSection(rel::Section::Strings, "strings",
              {imageData.strings.blob().size(), 8}, imageData.strings.count(),
              [&](Encoder& out) { out.raw(imageData.strings.blob()); });
  saveSection(rel::Section::Grammars, "grammars",
              {imageData.grammars.size(), rel::kGrammarBytes * 8},
              imageData.grammars.size(), [&](Encoder& out) {
                for (const latin::Analysis& grammar : imageData.grammars) {
                  out.u8(std::to_underlying(grammar.part));
                  out.u8(grammar.which);
                  out.u8(grammar.variant.value);
                  out.u8(std::to_underlying(grammar.caseOf));
                  out.u8(std::to_underlying(grammar.number));
                  out.u8(std::to_underlying(grammar.gender));
                  out.u8(std::to_underlying(grammar.comparison));
                  out.u8(std::to_underlying(grammar.numeralSort));
                  out.u8(std::to_underlying(grammar.tense));
                  out.u8(std::to_underlying(grammar.voice));
                  out.u8(std::to_underlying(grammar.mood));
                  out.u8(grammar.person.value);
                }
              });
  saveSection(rel::Section::Entries, "entries",
              {imageData.entries.size(), rel::kEntryBytes * 8},
              imageData.entries.size(), [&](Encoder& out) {
                for (const rel::Entry& entry : imageData.entries) {
                  out.u8(std::to_underlying(entry.part));
                  out.u8(entry.which);
                  out.u8(entry.variant);
                  out.u8(std::to_underlying(entry.gender));
                  out.u8(entry.kind);
                  out.u8(std::to_underlying(entry.comparison));
                  out.u8(std::to_underlying(entry.numeralSort));
                  out.u8(std::to_underlying(entry.area));
                  out.u8(std::to_underlying(entry.geography));
                  out.u8(std::to_underlying(entry.source));
                  out.u16(entry.numeralValue.value);
                }
              });

  image.fill(header, [&](Encoder& out) {
    for (const char c : rel::kMagic)
      out.u8(static_cast<std::uint8_t>(c));
    out.u32(rel::kVersion);
    out.u32(rel::kHeaderBytes);
    out.u64(image.byteSize());
    out.u32(rel::kSectionCount);
    out.u32(machine.root());
    out.u32(machine.rootOutput());
    out.u32(rel::kMaximumWordLength);
    out.u32(static_cast<std::uint32_t>(imageData.words.size()));
    out.u32(static_cast<std::uint32_t>(imageData.analyses.size()));
  });
  image.fill(directory, [&](Encoder& out) {
    for (const rel::DirectoryEntry& entry : entries) {
      out.u64(entry.offset);
      out.u64(entry.bytes);
      out.u64(entry.count);
    }
  });
  return image;
}

} // namespace emitter
