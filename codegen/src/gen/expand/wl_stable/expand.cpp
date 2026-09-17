#include "expand.hpp"

#include <print>

void wl_stable::expand::run(const tokenized::Sources& sources,
                            const text::StringPools& pools, Board& into) {
  spine::run<^^enrolled>(sources, pools, into);
  std::println("wl_stable expand has run");
}

void wl_stable::expand::post(const Board& board) {
  spine::post<^^enrolled>(board);
  std::println("wl_stable expand post has run");
}
