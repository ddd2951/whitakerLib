#include <string>

#include "facts.hpp"
#include "gen/expand/config.hpp"
#include "gen/expand/wl_stable/facts/slice.hpp"

wl_stable::expand::Slice wl_stable::expand::enrolled::slice() {
  Slice slice;
  for (const char letter : ::expand::config::kLetters) {
    const int index = facts::letterIndex(letter);
    if (index < 0)
      continue;
    const char canonical = static_cast<char>('a' + index);
    if (slice.letters.find(canonical) == std::string::npos)
      slice.letters += canonical;
  }
  return slice;
}
