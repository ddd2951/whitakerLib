#include <iostream>

#include <whitaker.h>

int main() {
  static WhitakerResult result;
  if (whitaker_init() != WHITAKER_OK)
    return 1;
  if (whitaker_analyze("amo", &result) != WHITAKER_OK)
    return 1;
  for (int i = 0; i < result.count; ++i) {
    const WhitakerMatch& match = result.matches[i];
    std::cout << match.orth << ": " << match.pos << " — " << match.meaning
              << '\n';
  }
}
