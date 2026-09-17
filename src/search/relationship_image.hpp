#pragma once

#include "relationship_schema.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#ifndef WHITAKER_EMBEDDED_ONLY
#include <filesystem>
#endif
#include <span>
#include <string>
#include <string_view>
#include <utility>
#ifndef WHITAKER_EMBEDDED_ONLY
#include <vector>
#endif

namespace whitaker::relationship {

class Image {
public:
  struct Program {
    std::uint32_t begin{};
    std::uint8_t count{};
  };

  struct Result {
    const char* orth{};
    const char* meaning{};
    const char* pos{};
    const char* inflection{};
  };
  struct Addon {
    const char* fix{};
    const char* meaning{};
    std::array<std::uint16_t, 4> target{};
    AddonKind kind{};
    Part root{};
    Part targetPart{};
    std::uint8_t rootKey{};
    std::uint8_t targetKey{};
    char connect{};
    std::uint8_t targetGate{};
    char firstRaw{};
    std::uint32_t rowStart{};
    std::uint16_t rowCount{};
  };
  struct FallbackRow {
    std::uint16_t inflect{};
    std::uint16_t description{};
  };
  struct Class {
    std::uint32_t rowStart{};
    std::uint16_t rowCount{};
    std::uint16_t gate{};
  };
  struct Inflection {
    const char* ending{};
    std::uint8_t key{};
    std::uint8_t allow{};
    std::uint8_t age{};
    std::uint8_t frequency{};
  };
  struct Description {
    const char* pos{};
    const char* inflection{};
  };
  struct FallbackStem {
    const char* text{};
    std::uint16_t dictionary{};
    std::uint8_t key{};
    Part part{};
  };
  struct DictionaryMetadata {
    std::uint8_t age{};
    std::uint8_t frequency{};
    std::uint16_t classId{};
  };

#ifndef WHITAKER_EMBEDDED_ONLY
  [[nodiscard]] bool load(const std::filesystem::path& path,
                          std::string& failure);
  [[nodiscard]] bool loadBytes(std::vector<std::byte> bytes,
                               std::string& failure);
#endif
  [[nodiscard]] bool loadSpan(std::span<const std::byte> bytes,
                              std::string& failure);
  [[nodiscard]] bool valid() const noexcept { return m_valid; }
  [[nodiscard]] Program lookup(std::string_view word) const noexcept;
  [[nodiscard]] Result next(std::uint32_t& cursor,
                            std::uint16_t& denseLexeme) const noexcept;
  [[nodiscard]] std::uint32_t addonCount() const noexcept;
  [[nodiscard]] Addon addon(std::uint32_t index) const noexcept;
  [[nodiscard]] std::uint32_t fallbackStemCount() const noexcept;
  [[nodiscard]] FallbackStem fallbackStem(std::uint32_t index) const noexcept;
  [[nodiscard]] std::pair<std::uint32_t, std::uint32_t>
  fallbackStemRange(std::string_view text) const noexcept;
  [[nodiscard]] DictionaryMetadata
  dictionaryMetadata(std::uint16_t index) const noexcept;
  [[nodiscard]] FallbackRow fallbackRow(std::uint32_t index) const noexcept;
  [[nodiscard]] Class dictionaryClass(std::uint16_t index) const noexcept;
  [[nodiscard]] const char*
  dictionaryMeaning(std::uint16_t index) const noexcept;
  [[nodiscard]] Inflection inflection(std::uint16_t index) const noexcept;
  [[nodiscard]] Description description(std::uint16_t index) const noexcept;
  [[nodiscard]] std::pair<std::uint16_t, std::uint16_t>
  endingRun(std::string_view text) const noexcept;
#ifndef WHITAKER_EMBEDDED_ONLY
  [[nodiscard]] std::vector<std::string> spellings() const;
#endif

  [[nodiscard]] std::size_t bytes() const noexcept { return m_bytes.size(); }
  [[nodiscard]] std::uint32_t spellingCount() const noexcept {
    return m_spellingCount;
  }
  [[nodiscard]] std::uint32_t resultCount() const noexcept {
    return m_resultCount;
  }
  [[nodiscard]] std::uint32_t maximumWordLength() const noexcept {
    return m_maximumWordLength;
  }
  [[nodiscard]] const DirectoryEntry& section(Section section) const noexcept;

private:
  [[nodiscard]] bool validate(std::string& failure);
  [[nodiscard]] bool validateHeader(std::string& failure);
  [[nodiscard]] bool validateDirectory(std::string& failure);
  [[nodiscard]] bool validateStates(std::string& failure) const;
  [[nodiscard]] bool validateLexemes(std::string& failure) const;
  [[nodiscard]] bool validateParadigms(std::string& failure) const;
  [[nodiscard]] bool validateRecords(std::string& failure) const;
  [[nodiscard]] bool validateStringPool(std::string& failure) const;
  [[nodiscard]] bool validateWalk(std::string& failure) const;
  [[nodiscard]] bool validateString(std::uint32_t offset,
                                    std::string& failure) const;
  [[nodiscard]] bool validateProgram(std::uint32_t begin, std::uint8_t count,
                                     std::uint32_t& end,
                                     std::string& failure) const;
  [[nodiscard]] const std::byte* at(Section section, std::uint64_t index,
                                    std::size_t width) const noexcept;
  [[nodiscard]] const char* stringAt(std::uint32_t offset) const noexcept;

#ifndef WHITAKER_EMBEDDED_ONLY
  std::vector<std::byte> m_ownedBytes;
#endif
  std::span<const std::byte> m_bytes;
  std::array<DirectoryEntry, kSectionCount> m_directory{};
  std::uint32_t m_root{};
  std::uint32_t m_rootOutput{};
  std::uint32_t m_maximumWordLength{};
  std::uint32_t m_spellingCount{};
  std::uint32_t m_resultCount{};
  bool m_valid{};
};

} // namespace whitaker::relationship
