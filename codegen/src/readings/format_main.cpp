#include "readings/format.hpp"
#include <fstream>

int main() {
  std::ofstream out("include/latin_generated.hpp", std::ios::binary);
  out << reading_format::kText;
  return out ? 0 : 1;
}
