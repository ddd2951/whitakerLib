#include <cstddef>
#include <vector>

#include "facts.hpp"
#include "expand/common/reading.hpp"
#include "expand/facts/readings.hpp"

expand::Readings expand::readings(const Forms& listed) {
  Readings readings;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::vector<Form>& forms = listed.byLetter[letter];
    std::vector<Reading>& out = readings.byLetter[letter];
    out.reserve(forms.size());
    for (const Form& form : forms)
      out.push_back(readingOf(form.origin));
  }
  return readings;
}
