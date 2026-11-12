#pragma once

// INFO: Gen's steps run by reflection: every function declared under a namespace of steps.hpp. A step waits for its
//  gates, and for every step declared before it that shares a value with it (tag::Reads, tag::Writes), one of the two
//  writing it. Checked at compile time: every gate names a step, and no gate waits on itself through others.
#include "execution_steps/steps.hpp"
#include "execution_steps/tags.hpp"

#include <algorithm>
#include <array>
#include <condition_variable>
#include <cstddef>
#include <meta>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace runner {

using std::meta::info;

consteval std::vector<info> stepsIn(const info scope) {
  std::vector<info> steps{};
  for (const info member : std::meta::members_of(scope, std::meta::access_context::current())) {
    if (std::meta::is_namespace(member)) {
      const auto nested = stepsIn(member);
      steps.insert(steps.end(), nested.begin(), nested.end());
    } else if (std::meta::is_function(member)) {
      steps.push_back(member);
    }
  }
  return steps;
}

consteval std::vector<info> gatesOf(const info step) {
  std::vector<info> gates{};
  for (const info annotation : std::meta::annotations_of_with_type(step, ^^execution_steps::tag::Gate))
    gates.push_back(std::meta::extract<execution_steps::tag::Gate>(annotation).step);
  return gates;
}

struct Access {
  info value{};
  bool writes{};
};

consteval std::vector<Access> accessesOf(const info step) {
  std::vector<Access> accesses{};
  for (const info annotation : std::meta::annotations_of_with_type(step, ^^execution_steps::tag::Reads))
    accesses.push_back({std::meta::extract<execution_steps::tag::Reads>(annotation).value, false});
  for (const info annotation : std::meta::annotations_of_with_type(step, ^^execution_steps::tag::Writes))
    accesses.push_back({std::meta::extract<execution_steps::tag::Writes>(annotation).value, true});
  return accesses;
}

// INFO: Whether `later` must wait for `earlier`: they share a value and one of them writes it.
consteval bool conflict(const std::vector<Access>& earlier, const std::vector<Access>& later) {
  for (const Access a : earlier)
    for (const Access b : later)
      if (a.value == b.value && (a.writes || b.writes))
        return true;
  return false;
}

// INFO: For each step under `scope`, in declaration order, the places of the steps under `scope` it waits for: its
//  gates, and the earlier steps it conflicts with. A gate outside `scope` has run before it.
consteval std::vector<std::vector<std::size_t>> waitsOf(const std::vector<info>& steps) {
  std::vector<std::vector<std::size_t>> waits(steps.size());
  // NOTE: Each step's tags read once; reading them per pair exceeds GCC's constexpr operation limit.
  std::vector<std::vector<Access>> accesses{};
  for (const info step : steps)
    accesses.push_back(accessesOf(step));
  for (std::size_t i = 0; i < steps.size(); ++i) {
    for (const info gate : gatesOf(steps[i]))
      if (const auto at = std::ranges::find(steps, gate); at != steps.end())
        waits[i].push_back(static_cast<std::size_t>(at - steps.begin()));
    for (std::size_t j = 0; j < i; ++j)
      if (conflict(accesses[j], accesses[i]) && std::ranges::find(waits[i], j) == waits[i].end())
        waits[i].push_back(j);
  }
  return waits;
}

// INFO: The steps under `scope` in an order every wait allows: each time, the first declared step whose waits have
//  run. Empty when a gate names no step or the gates have a cycle.
consteval std::vector<info> ordered(const info scope) {
  const std::vector<info> all = stepsIn(^^execution_steps);
  for (const info step : all)
    for (const info gate : gatesOf(step))
      if (std::ranges::find(all, gate) == all.end())
        return {};
  const std::vector<info> steps = stepsIn(scope);
  const auto waits = waitsOf(steps);
  std::vector<bool> done(steps.size(), false);
  std::vector<info> order{};
  while (order.size() < steps.size()) {
    bool advanced = false;
    for (std::size_t i = 0; i < steps.size() && !advanced; ++i) {
      if (done[i] || !std::ranges::all_of(waits[i], [&](const std::size_t w) { return done[w]; }))
        continue;
      done[i] = true;
      order.push_back(steps[i]);
      advanced = true;
    }
    if (!advanced)
      return {};
  }
  return order;
}

// INFO: waitsOf flattened for a constant: how many each step waits for, then their places in `order`.
consteval std::vector<std::size_t> waitCounts(const info scope) {
  const std::vector<info> order = ordered(scope);
  std::vector<std::size_t> counts{};
  for (const auto& w : waitsOf(order))
    counts.push_back(w.size());
  return counts;
}

consteval std::vector<std::size_t> waitPlaces(const info scope) {
  const std::vector<info> order = ordered(scope);
  std::vector<std::size_t> places{};
  for (const auto& w : waitsOf(order))
    places.insert(places.end(), w.begin(), w.end());
  return places;
}

template <info scope> void run() {
  static constexpr auto order = std::define_static_array(ordered(scope));
  static_assert(order.size() == stepsIn(scope).size(), "every gate must name a step, and the gates must have no cycle");
  template for (constexpr info step : order) { [:step:](); }
}

// INFO: The steps under `scope`, each started on one of `threads` threads once all it waits for under `scope` has
//  finished. Exact only if the tags are complete: two steps nothing orders may run at once.
template <info scope> void runParallel(const unsigned threads = std::thread::hardware_concurrency()) {
  static constexpr auto order = std::define_static_array(ordered(scope));
  static_assert(order.size() == stepsIn(scope).size(), "every gate must name a step, and the gates must have no cycle");
  static constexpr auto counts = std::define_static_array(waitCounts(scope));
  static constexpr auto places = std::define_static_array(waitPlaces(scope));
  static constexpr auto calls = []<std::size_t... i>(std::index_sequence<i...>) {
    return std::array<void (*)(), sizeof...(i)>{&[:order[i]:]...};
  }(std::make_index_sequence<order.size()>{});

  std::vector<std::size_t> remaining(counts.begin(), counts.end());
  std::vector<std::vector<std::size_t>> waiting(order.size());
  for (std::size_t step = 0, at = 0; step < order.size(); ++step)
    for (std::size_t k = 0; k < counts[step]; ++k)
      waiting[places[at++]].push_back(step);

  std::mutex lock{};
  std::condition_variable changed{};
  std::vector<std::size_t> ready{};
  std::size_t finished = 0;
  for (std::size_t step = 0; step < order.size(); ++step)
    if (remaining[step] == 0)
      ready.push_back(step);

  const auto work = [&] {
    std::unique_lock held{lock};
    for (;;) {
      changed.wait(held, [&] { return !ready.empty() || finished == order.size(); });
      if (ready.empty())
        return;
      const std::size_t step = ready.back();
      ready.pop_back();
      held.unlock();
      calls[step]();
      held.lock();
      ++finished;
      for (const std::size_t next : waiting[step])
        if (--remaining[next] == 0)
          ready.push_back(next);
      changed.notify_all();
    }
  };
  std::vector<std::jthread> pool{};
  for (unsigned t = 0; t < std::max(1u, threads); ++t)
    pool.emplace_back(work);
}

// INFO: Several phases, one after another.
template <info first, info second, info... rest> void run() {
  run<first>();
  run<second>();
  (run<rest>(), ...);
}

} // namespace runner
