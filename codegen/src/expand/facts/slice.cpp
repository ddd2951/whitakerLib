#include <string>

#include "facts.hpp"
#include "expand/config.hpp"
#include "expand/facts/slice.hpp"

expand::Slice expand::slice() {
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
