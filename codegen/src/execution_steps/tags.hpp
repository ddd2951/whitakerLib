#pragma once

#include <meta>
#include <type_traits>

namespace execution_steps::tag {

struct Gate {
  std::meta::info step{};
};
static_assert(std::is_trivially_copyable_v<Gate>);

// INFO: A value a step reads, or writes (changes in any way, which includes reading it); one value an annotation.
//  Two steps that share a value, one of them writing it, run in declaration order.
struct Reads {
  std::meta::info value{};
};
static_assert(std::is_trivially_copyable_v<Reads>);

struct Writes {
  std::meta::info value{};
};
static_assert(std::is_trivially_copyable_v<Writes>);

} // namespace execution_steps::tag
