#include "lines.hpp"

int main(int argc, char** argv) {
  try {
    if (argc < 3) {
      std::cerr << "usage: lines CAPTURES_DIR OUT_DIR [BOOK...]\n"
                   "       lines --show CAPTURES_DIR WORD...\n"
                   "       lines --probe CAPTURES_DIR [BOOK...]\n"
                   "       lines --import CAPTURES_DIR OUT_DIR\n";
      return 2;
    }
    if (std::string_view(argv[1]) == "--show")
      return lines::show(argv[2], {argv + 3, argv + argc});
    if (std::string_view(argv[1]) == "--probe")
      return lines::probe(argv[2], {argv + 3, argv + argc});
    if (std::string_view(argv[1]) == "--import") {
      if (argc != 4)
        throw std::runtime_error("--import needs a capture set and an output directory");
      return lines::importAnswers(argv[2], argv[3]);
    }
    return lines::run(argv[1], argv[2], {argv + 3, argv + argc});
  } catch (const std::exception& error) {
    std::cerr << "lines: " << error.what() << '\n';
    return 1;
  }
}
