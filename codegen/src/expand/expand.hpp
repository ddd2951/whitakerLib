#pragma once

#include "expand/common/form.hpp"
#include "expand/common/reading.hpp"
#include "expand/facts/swept.hpp"
#include "shared/semantic/value.hpp"

#include <cstddef>
#include <string_view>

namespace expand {

struct ResultTag;

using ResultIndex = semantic::Value<ResultTag, std::size_t>;

void init();

[[nodiscard]] std::size_t resultCount();

class Result {
public:
  explicit constexpr Result(ResultIndex index) : index{index} {}

  [[nodiscard]] std::string_view spelling() const;
  [[nodiscard]] Origin origin() const;
  [[nodiscard]] Reading reading() const;
  [[nodiscard]] Fate fate() const;

private:
  ResultIndex index;
};

} // namespace expand
