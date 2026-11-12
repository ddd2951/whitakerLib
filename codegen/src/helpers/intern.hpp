#pragma once

#include "storage/rows.hpp"
#include "storage/rules.hpp"

#include <utility>

namespace helpers {

inline unsigned intern(storage::RuleKey key) {
  using storage::rows;
  using storage::rules;
  const auto [it, fresh] = rules.emplace(key, rows.size());
  if (fresh)
    rows.push_back(std::move(key));
  return it->second;
}

} // namespace helpers
