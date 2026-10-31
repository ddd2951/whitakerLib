#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace lines {

struct Record {
  std::array<std::string, 4> stems;
  std::string part;
  int which = 0, variant = 0;
  std::string gender, kind, comparison, object;
  // INFO: age, area, geography, frequency, source: XXXCO
  std::string codes;
  std::string meaning;
};

// INFO: DICTLINE.GEN's records in order, then the esse makedict adds, then UNIQUES.LAT's: the order WORDS numbers them
//  in and whitakerLib's entry.id follows.
class Dictionary {
public:
  explicit Dictionary(const std::filesystem::path& data);
  [[nodiscard]] const Record* record(std::uint16_t id) const;
  [[nodiscard]] std::size_t general() const { return m_general; }

private:
  std::vector<Record> m_records;
  std::size_t m_general = 0;
};

std::string dictionaryForm(const Record& record);

} // namespace lines
