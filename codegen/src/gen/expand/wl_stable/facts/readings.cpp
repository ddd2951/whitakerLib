#include <cstddef>
#include <vector>

#include "facts.hpp"
#include "gen/expand/wl_stable/common/reading.hpp"
#include "gen/expand/wl_stable/facts/readings.hpp"

wl_stable::expand::Readings
wl_stable::expand::enrolled::readings(const tokenized::Sources& sources,
                                      const Synthetics& synthetics,
                                      const Forms& listed) {
  Readings readings;
  for (std::size_t letter = 0; letter < facts::kLetterCount; ++letter) {
    const std::vector<Form>& forms = listed.byLetter[letter];
    std::vector<Reading>& out = readings.byLetter[letter];
    out.reserve(forms.size());
    for (const Form& form : forms)
      out.push_back(readingOf(sources, synthetics, form.origin));
  }
  return readings;
}
