#include "emitter/emitter.hpp"
#include "expand/expand.hpp"
#include "source/source.hpp"
#include "word/word.hpp"

int main() {
  // NOTE: Nothing checks this order; reordering breaks generation.
  source::init();
  word::init();
  expand::init();
  emitter::emit();
  return 0;
}
