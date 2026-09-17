#pragma once

#include <cstddef>
#include <memory>
#include <meta>
#include <tuple>
#include <utility>
#include <vector>

#include "gen/expand/wl_stable/spine/members.hpp"
#include "gen/expand/wl_stable/spine/rules.hpp"
#include "types/string_pools.hpp"
#include "types/tokenized_sources.hpp"

namespace wl_stable::expand::spine {

// NOTE: A slot is null until its fact has run.
consteval std::meta::info slotOf(std::meta::info fact) {
  return std::meta::substitute(^^std::unique_ptr,
                               {
                                   std::meta::return_type_of(fact)});
}

template <std::meta::info Space> consteval std::meta::info boardOf() {
  std::vector<std::meta::info> slots;
  for (const auto fact : kFacts<Space>)
    slots.push_back(slotOf(fact));
  return std::meta::substitute(^^std::tuple, slots);
}

template <std::meta::info Space> using Board = [:boardOf<Space>():];

template <typename T, typename Board>
[[nodiscard]] const T& read(const tokenized::Sources& sources,
                            const text::StringPools& pools,
                            const Board& board) {
  if constexpr (^^T == ^^tokenized::Sources)
    return sources;
  else if constexpr (^^T == ^^text::StringPools)
    return pools;
  else
    return *std::get<std::unique_ptr<T>>(board);
}

template <std::meta::info Fact>
inline constexpr auto kReadsOf =
    std::define_static_array(std::meta::parameters_of(Fact));

template <std::meta::info Fact, typename Board, std::size_t... I>
[[nodiscard]] auto invoke(const tokenized::Sources& sources,
                          const text::StringPools& pools, const Board& board,
                          std::index_sequence<I...>) {
  return [:Fact:](
      read<typename[:readOf(kReadsOf<Fact>[I]):]>(sources, pools, board)...);
}

template <std::meta::info Space>
void run(const tokenized::Sources& sources, const text::StringPools& pools,
         Board<Space>& board) {
  template for (constexpr auto fact : kFacts<Space>) {
    using Result = [:std::meta::return_type_of(fact):];
    std::get<std::unique_ptr<Result>>(board) = std::make_unique<Result>(
        invoke<fact>(sources, pools, board,
                     std::make_index_sequence<kReadsOf<fact>.size()>{}));
  }
}

template <std::meta::info Space> void post(const Board<Space>& board) {
  template for (constexpr auto report : kReports<Space>) {
    using Fact = [:readOf(kReadsOf<report>[0]):];
    [:report:](*std::get<std::unique_ptr<Fact>>(board));
  }
}

} // namespace wl_stable::expand::spine
